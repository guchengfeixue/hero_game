@echo off
if not exist ..\cmd_build mkdir ..\cmd_build
pushd ..\cmd_build
cl -DHEROGAME_INTERNAL=1 -DHEROGAME_SLOW=1 -DHEROGAME_WIN32=1 -FC -Zi ..\herogame\win32_herogame.cpp user32.lib Gdi32.lib
popd
