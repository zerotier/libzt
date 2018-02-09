cd ../../
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=DEBUG
cmake --build build
copy bin\lib\Debug\*.lib packages\pypi
cd packages\pypi
pip3 install wheel twine
py setup.py bdist_wheel	