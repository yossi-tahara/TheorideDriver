@echo off

openfiles > NUL 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo 管理者権限で実行して下さい。
    goto End 
)

set COMPILER_PATH=
for /f "usebackq delims=" %%a in (`where /F cl.exe`) do set COMPILER_PATH=%%a
echo %COMPILER_PATH%

call impl.bat %COMPILER_PATH% replace HostX64\x64
call impl.bat %COMPILER_PATH% replace HostX64\x86
call impl.bat %COMPILER_PATH% replace HostX86\x64
call impl.bat %COMPILER_PATH% replace HostX86\x86

:End
