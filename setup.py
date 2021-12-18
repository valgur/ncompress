# based on https://github.com/pybind/python_example/blob/1f4f73582cbfc2a0690b3930680abeab39820c03/setup.py

# pybind11 is available at setup time thanks to pyproject.toml
import os

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "1.0.0"


class build_ext_custom(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            if self.compiler.compiler_type == "msvc":
                cflags = os.environ.get("CFLAGS")
                if cflags:
                    ext.extra_compile_args = list(cflags.split(" "))
            else:
                ext.extra_compile_args = ["-O3", "-Wall", "-Wextra", "-Wpedantic"]
            if self.compiler.compiler_type == "unix":
                # workaround for ‘pystream::streambuf’ declared with greater visibility than the type of its field
                # ‘pystream::streambuf::py_read’ [-Werror=attributes]
                ext.extra_compile_args += ["-fvisibility=hidden"]
        super().build_extensions()


ext_modules = [
    Pybind11Extension(
        "ncompress",
        sources=["src/ncompress.cpp", "src/python.cpp"],
        include_dirs=["include"],
        define_macros=[("VERSION_INFO", __version__)],
    )
]

setup(
    name="ncompress",
    version=__version__,
    author="Martin Valgur",
    author_email="martin.valgur@gmail.com",
    url="https://github.com/valgur/ncompress",
    description="LZW compression and decompression",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": ["pytest"]},
    cmdclass={"build_ext": build_ext_custom},
    zip_safe=False,
    python_requires=">=3.6",
)
