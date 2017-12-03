setlocal

set OPE_MODE=%2
set BUILD_MODE=%3
set COMPILER="%~d1%~p1..\..\%BUILD_MODE%\cl.exe"
echo %COMPILER%

if exist %COMPILER% (
    build\bin\TheorideDriver.exe --theoride-%OPE_MODE%=%COMPILER%
)

endlocal
