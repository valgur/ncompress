# ncompress [![PyPI](https://img.shields.io/pypi/v/ncompress)](https://pypi.org/project/ncompress/) [![Build](https://github.com/valgur/velodyne_decoder/actions/workflows/build.yml/badge.svg?event=push)](https://github.com/valgur/velodyne_decoder/actions/workflows/build.yml) [![PyPI - Downloads](https://img.shields.io/pypi/dm/ncompress)](https://pypistats.org/packages/ncompress)

LZW compression and decompression in Python and C++.

Ported with minimal changes from the [(N)compress](https://github.com/vapier/ncompress) CLI tool.

## Installation

Wheels are available for Python 3.8+ and all operating systems on PyPI.

```bash
pip install ncompress
```

## Usage

Functions `compress()` and `decompress()` are available with the following inputs/outputs:

* `bytes` → `bytes`
* `BytesIO` → `bytes`
* `BytesIO`, `BytesIO` → `None`
* `bytes`, `BytesIO` → `None`

The `BytesIO`-based functions are slightly (about 15%) faster due to avoiding a copy of the contents on `bytes`⇄`std::string` conversion.

## Authors

* Martin Valgur ([@valgur](https://github.com/valgur))

The core functionality has been adapted from [vapier/ncompress](https://github.com/vapier/ncompress).

## License

All modifications and additions are released under the [Unlicense](LICENSE).

The base (N)compress code has been released into the public domain.

The `BytesIO` wrapper in [pystreambuf.h](src/pystreambuf.h) has been adapted from the [cctbx](https://github.com/cctbx/cctbx_project) project and is licensed under the Lawrence Berkeley National Labs BSD variant license.
See the linked file for details.
