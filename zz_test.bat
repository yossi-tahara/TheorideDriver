call "N:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

cd build
cmake --build . --config Release
ctest -C Release -V

pause
