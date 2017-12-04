@echo off

%~d0
cd  %~dp0

openfiles > NUL 2>&1
if NOT %ERRORLEVEL% EQU 0 (
    echo ŠÇ—ÒŒ ŒÀ‚ÅÀs‚µ‚Ä‰º‚³‚¢B
    goto End 
)

call bin\2017.bat replace
rem call bin\2015.bat replace

:End

pause
