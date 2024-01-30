# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2] - 2024-01-30

Re-release of 1.0.1 to fix an incomplete source distribution being uploaded to PyPI.

## [1.0.1] - 2023-04-13

### Python bindings

* No changes to the API.
* Replaced [`pybind11`](https://github.com/pybind/pybind11) with [`nanobind`](https://github.com/wjakob/nanobind) for smaller wheels (i.e. 100 kB instead of 900 kB for manylinux wheels) and slightly improved performance.
* The minimal Python version is now 3.8 instead of 3.6 and 32-bit systems are no longer supported due to constraints from nanobind.
* Wheels are now built for all supported architectures, not just x86_64.
* Added `cmake` and `ninja` as build dependencies and `cibuildwheel` config to `pyproject.toml`.
  This allows building from source with `pip install ncompress` and building wheels using the `cibuildwheel` command to work out of the box.
* Custom logic in `setup.py` was replaced with `scikit-build`.

### C++ library

* No changes to the API.
* Eliminated C-style #defines and macros.
* Reduced the scope of C-style variable declarations for readability.
* Added `MAGIC_1` and `MAGIC_2` constants at the start of compressed files to `ncompress.h` for convenience.

## [1.0.0] - 2021-12-19

First release.

The core functionality was adapted from [(N)compress](https://github.com/vapier/ncompress) with minimal changes 
to make it work smoothly with C++ and Python without introducing any new bugs.

[1.0.2]: https://github.com/valgur/ncompress/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/valgur/ncompress/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/valgur/ncompress/releases/tag/v1.0.0