@echo off

%~d0
cd  %~dp0

openfiles > NUL 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo �Ǘ��Ҍ����Ŏ��s���ĉ������B
    goto End 
)

call bin\2017.bat restore
call bin\2015.bat restore

:End

pause
