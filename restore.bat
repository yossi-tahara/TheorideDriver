@echo off

%~d0
cd  %~dp0

openfiles > NUL 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo 管理者権限で実行して下さい。
    goto End 
)

call bin\2017.bat restore
rem call bin\2015.bat restore
rem call bin\2013.bat restore

:End

pause
