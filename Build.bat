@echo off

if "%~1"=="" (
    echo Usage: %0 [Unreal Engine Installation Directory]
    exit /b 1
)

set "UNREAL_ENGINE_PATH=%~1"

"%UNREAL_ENGINE_PATH%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%CD%\Mythica.uplugin" -Package="%CD%\Build\Mythica" -nocompile -nocompileuat
