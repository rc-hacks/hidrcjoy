@echo off

if "%AVR8_GNU_TOOLCHAIN%"=="" (
    echo ERROR: Environment variable 'AVR8_GNU_TOOLCHAIN' not defined.
    exit /B 1
)

if "%AVR8_TOOLS%"=="" (
    echo ERROR: Environment variable 'AVR8_TOOLS' not defined.
    exit /B 1
)

if "%AVR8_SETUP%"=="" set path=%AVR8_TOOLS%\bin;%AVR8_GNU_TOOLCHAIN%\bin;%path%
set AVR8_SETUP=true

make.exe %*
