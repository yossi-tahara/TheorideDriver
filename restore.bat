@echo off

openfiles > NUL 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo ä«óùé“å†å¿Ç≈é¿çsÇµÇƒâ∫Ç≥Ç¢ÅB
    goto End 
)

set COMPILER_PATH=
for /f "usebackq delims=" %%a in (`where /F cl.exe`) do set COMPILER_PATH=%%a
echo %COMPILER_PATH%

call impl.bat %COMPILER_PATH% restore HostX64\x64
call impl.bat %COMPILER_PATH% restore HostX64\x86
call impl.bat %COMPILER_PATH% restore HostX86\x64
call impl.bat %COMPILER_PATH% restore HostX86\x86

:End
