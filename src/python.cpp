#include <istream>
#include <sstream>
#include <string>

#include <nanobind/nanobind.h>

#include <bytesobject.h>
#include <pyerrors.h>

#include "ncompress.h"
#include "pystreambuf.h"

namespace nb = nanobind;
using pystream::streambuf;

// Unlike pybind11, nanobind doesn't provide a bytes <-> std::string type caster.
namespace nanobind::detail
{
template <> struct type_caster<std::string>
{
  NB_TYPE_CASTER(std::string, const_name("bytes"));

  bool from_python(handle src, uint8_t, cleanup_list *) noexcept
  {
    if (!PyBytes_Check(src.ptr()))
    {
      PyErr_Clear();
      return false;
    }
    char *str;
    Py_ssize_t size;
    PyBytes_AsStringAndSize(src.ptr(), &str, &size);
    if (!str)
    {
      PyErr_Clear();
      return false;
    }
    value = std::string(str, (size_t)size);
    return true;
  }

  static handle from_cpp(const std::string &value, rv_policy, cleanup_list *) noexcept
  {
    return PyBytes_FromStringAndSize(value.c_str(), value.size());
  }
};
} // namespace nanobind::detail

NB_MODULE(ncompress_core, m)
{
  // io.BytesIO input-output
  m.def("compress", ncompress::compress, nb::arg("in_stream"), nb::arg("out_stream"));
  m.def("decompress", ncompress::decompress, nb::arg("in_stream"), nb::arg("out_stream"));

  // bytes input, io.BytesIO output
  m.def(
      "compress",
      [](const std::string &data, std::ostream &out) {
        std::istringstream in(data);
        ncompress::compress(in, out);
      },
      nb::arg("in_bytes"), nb::arg("out_stream"));
  m.def(
      "decompress",
      [](const std::string &data, std::ostream &out) {
        std::istringstream in(data);
        ncompress::decompress(in, out);
      },
      nb::arg("in_bytes"), nb::arg("out_stream"));

  // io.BytesIO input, bytes output
  m.def(
      "compress",
      [](std::istream &in) -> std::string {
        std::ostringstream out;
        ncompress::compress(in, out);
        return out.str();
      },
      nb::arg("in_stream"));
  m.def(
      "decompress",
      [](std::istream &in) -> std::string {
        std::ostringstream out;
        ncompress::decompress(in, out);
        return out.str();
      },
      nb::arg("in_stream"));

  // bytes input-output
  m.def(
      "compress",
      [](const std::string &data) -> std::string {
        std::istringstream in(data);
        std::ostringstream out;
        ncompress::compress(in, out);
        return out.str();
      },
      nb::arg("in_bytes"));
  m.def(
      "decompress",
      [](const std::string &data) -> std::string {
        std::istringstream in(data);
        std::ostringstream out;
        ncompress::decompress(in, out);
        return out.str();
      },
      nb::arg("in_bytes"));
}
