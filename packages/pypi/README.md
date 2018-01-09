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

## Building the Package 

Package is specified in [setup.py](setup.py). *Note: A new version must be specified in `setup.py` for every new upload of a package*

Get necessary tools

`pip3 install wheel twine`

*Note: As there appears to be no way to differentiate C and C++ code (and thus pass correct build args to each type) in a setuptools script we must separately build the `lwip` and `http_parser` libraries, copy them here, and then build the binary. See the top-level [README.md](../../README.md) for instructions on how to do that*

Build the binary distribution wheel:

`python3 setup.py bdist_wheel`

Upload to PyPI

`twine upload dist/*`