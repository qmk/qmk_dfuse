@echo off
setlocal
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set arch=x86 || set arch=x64
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set arch2=x86 || set arch2=amd64

for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSIONNR=%%i.%%j

if "%versionnr%" == "10.0" set version=Win10
if "%versionnr%" == "6.3" set version=Win8.1 
if "%versionnr%" == "6.2" set version=Win8
if "%versionnr%" == "6.1" set version=Win7


pushd "%~dp0" 
call %version%\%arch%\dpinst_%arch2%.exe
popd

endlocal