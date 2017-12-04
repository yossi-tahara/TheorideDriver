@echo off
chcp 65001

setlocal

set OPE_MODE=%2

rem vcvars.bat
call %1 %~3 >NUL 2>&1

set COMPILER_PATH=
for /f "usebackq delims=" %%a in (`where /F cl.exe`) do (
    set COMPILER_PATH=%%~a
    goto break
)
:break
rem echo COMPILER_PATH=%COMPILER_PATH%

bin\cl.exe "--theoride-%OPE_MODE%=%COMPILER_PATH%"

endlocal
