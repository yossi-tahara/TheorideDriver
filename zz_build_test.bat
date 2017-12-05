cd build
cmake --build . --config Release --target Install

if %ERRORLEVEL% == 0 (
    call "N:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
rem chcp 65001
    ctest -C Release -V
)
pause
