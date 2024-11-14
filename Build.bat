@echo off

if "%~1"=="" (
    echo Usage: %0 [Unreal Engine Installation Directory]
    exit /b 1
)

for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set dt=%%I
set Timestamp=%dt:~0,8%_%dt:~8,6%
set BuildDir=%CD%\Builds\Build_%Timestamp%

call "%~1\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%CD%\Mythica.uplugin" -Package="%BuildDir%\Mythica" -nocompile -nocompileuat

del "%BuildDir%\Mythica\Binaries\Win64\*.pdb"

powershell Compress-Archive -Path "%BuildDir%\Mythica" -DestinationPath "%BuildDir%\Mythica.zip"