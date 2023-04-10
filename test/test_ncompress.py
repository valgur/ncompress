import shutil
import subprocess
from io import BytesIO

import pytest
from ncompress import compress, decompress


@pytest.fixture
def sample_data():
    chars = []
    for i in range(15):
        chars += [i * 16] * (i + 1)
    chars += [0, 0, 0]
    return bytes(chars)


@pytest.fixture
def sample_compressed(sample_data):
    compress_cmd = shutil.which("compress")
    if compress_cmd:
        return subprocess.check_output(compress_cmd, input=sample_data)
    return compress(sample_data)


def test_string_string(sample_data, sample_compressed):
    assert compress(sample_data) == sample_compressed
    assert decompress(sample_compressed) == sample_data


def test_string_stream(sample_data, sample_compressed):
    out = BytesIO()
    compress(sample_data, out)
    out.seek(0)
    assert out.read() == sample_compressed

    out = BytesIO()
    decompress(sample_compressed, out)
    out.seek(0)
    assert out.read() == sample_data


def test_stream_stream(sample_data, sample_compressed):
    out = BytesIO()
    compress(BytesIO(sample_data), out)
    out.seek(0)
    assert out.read() == sample_compressed

    out = BytesIO()
    decompress(BytesIO(sample_compressed), out)
    out.seek(0)
    assert out.read() == sample_data


def test_stream_string(sample_data, sample_compressed):
    assert compress(BytesIO(sample_data)) == sample_compressed
    assert decompress(BytesIO(sample_compressed)) == sample_data


def test_empty_input(sample_data):
    assert decompress(compress(b"")) == b""
    with pytest.raises(ValueError):
        decompress(b"")
    with pytest.raises(TypeError):
        compress()
    with pytest.raises(TypeError):
        decompress()


def test_corrupted_input(sample_compressed):
    sample = sample_compressed
    for x in [
        b"123",
        sample[1:],
        sample[:1],
        b"\0" * 3 + sample[:3],
        sample * 2,
        b"\0" + sample
    ]:
        with pytest.raises(ValueError) as ex:
            decompress(x)
        assert ("not in LZW-compressed format" in str(ex.value) or
                "corrupt input - " in str(ex.value))


def test_invalid_input():
    with pytest.raises(TypeError):
        compress("abc")
    with pytest.raises(TypeError):
        decompress("abc")
    with pytest.raises(TypeError):
        compress()
    with pytest.raises(TypeError):
        decompress()
    with pytest.raises(TypeError):
        compress(None)
    with pytest.raises(TypeError):
        decompress(None)
    with pytest.raises(TypeError):
        compress(0)
    with pytest.raises(TypeError):
        decompress(0)


def test_closed_input(sample_data, sample_compressed):
    expected = "I/O operation on closed file."
    with pytest.raises(ValueError) as ex:
        stream = BytesIO(sample_data)
        stream.close()
        compress(stream)
    assert expected in str(ex.value)

    with pytest.raises(ValueError) as ex:
        stream = BytesIO(sample_compressed)
        stream.close()
        decompress(stream)
    assert expected in str(ex.value)


def test_file_input():
    with open(__file__, "rb") as f:
        expected = f.read()
        f.seek(0)
        assert decompress(compress(f)) == expected
