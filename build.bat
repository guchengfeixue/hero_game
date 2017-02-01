@echo off
mkdir ..\cmd_build
pushd ..\cmd_build
cl -Zi ..\herogame\win32_herogame.cpp user32.lib Gdi32.lib
popd
