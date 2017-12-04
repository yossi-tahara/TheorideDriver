setlocal

echo "--- Visual Studio 2013 ---"

set VS=Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat
if exist "C:\%VS%" set VS="C:\%VS%" & goto Skip
if exist "D:\%VS%" set VS="D:\%VS%" & goto Skip
if exist "M:\%VS%" set VS="M:\%VS%" & goto Skip
if exist "N:\%VS%" set VS="N:\%VS%" & goto Skip
goto Non

:Skip

cmd.exe /C bin\impl.bat %VS% %1 ""
cmd.exe /C bin\impl.bat %VS% %1 amd64
cmd.exe /C bin\impl.bat %VS% %1 amd64_x86
cmd.exe /C bin\impl.bat %VS% %1 x86_amd64

:Non

endlocal
