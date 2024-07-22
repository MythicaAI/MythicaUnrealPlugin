@echo off

if "%~1"=="" (
    echo Usage: %0 [Unreal Engine Installation Directory]
    exit /b 1
)

call "%~1\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%CD%\Mythica.uplugin" -Package="%CD%\Build\Mythica" -nocompile -nocompileuat

del %CD%\Build\Mythica\Binaries\Win64\*.pdb

powershell Compress-Archive -Path "%CD%\Build\Mythica" -DestinationPath "%CD%\Build\Mythica.zip"