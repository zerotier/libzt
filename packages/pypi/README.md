## libzt PyPI Package
[pypi/libzt](https://pypi.python.org/pypi/libzt)
***

## Getting started

`pip3 install libzt`

```
import libzt
import time

print('joining virtual network...')
libzt.zts_startjoin('whatev_config', 0x123456789ABCDEF1)
print('fd = ' + str(libzt.zts_socket(1,2,3)))

print('looping forever, ping me')
while True:
    time.sleep(1)
```

## Building

### macOS, Linux

Binary distribution: 

make bdist

Alternatively, source Distribution: 

make sdist

Upload to PyPI: 

make upload

Cleanup:

make clean

### Windows

Binary distribution:

bdist.bat

Source distribution:

sdist.bat

Upload to PyPI:

upload.bat

Cleanup:

clean.bat


*Note: As there appears to be no way to differentiate C and C++ code (and thus pass correct build args to each type) in a setuptools script we must separately build the `lwip` and `http_parser` libraries, copy them here, and then build the binary. See the top-level [README.md](../../README.md) for instructions on how to do that*
