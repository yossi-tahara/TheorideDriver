call "N:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

cd build
cmake --build . --config Release
if %ERRORLEVEL% == 0 (
    ctest -C Release -V
)
pause
