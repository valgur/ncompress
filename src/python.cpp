#include <sstream>
#include <string>

#include <pybind11/pybind11.h>

#include "ncompress.h"
#include "pystreambuf.h"

namespace py = pybind11;

PYBIND11_MODULE(ncompress, m)
{
  m.doc() = "";

  // io.BytesIO input-output
  m.def("compress", &ncompress::compress);
  m.def("decompress", &ncompress::decompress);

  // bytes input-output
  m.def("compress", [](const py::bytes &data) -> py::bytes {
    std::stringstream in(data);
    std::stringstream out;
    ncompress::compress(in, out);
    return out.str();
  });
  m.def("decompress", [](const py::bytes &data) -> py::bytes {
    std::stringstream in(data);
    std::stringstream out;
    ncompress::decompress(in, out);
    return out.str();
  });

  // io.BytesIO input, bytes output
  m.def("compress", [](std::istream &in) -> py::bytes {
    std::stringstream out;
    ncompress::compress(in, out);
    return out.str();
  });
  m.def("decompress", [](std::istream &in) -> py::bytes {
    std::stringstream out;
    ncompress::decompress(in, out);
    return out.str();
  });

#define STRING(s) #s
#ifdef VERSION_INFO
  m.attr("__version__") = STRING(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
