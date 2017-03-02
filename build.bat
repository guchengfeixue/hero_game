@echo off
if not exist ..\cmd_build mkdir ..\cmd_build
pushd ..\cmd_build
cl -nologo -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHEROGAME_INTERNAL=1 -DHEROGAME_SLOW=1 -DHEROGAME_WIN32=1 -FC -Z7 ..\herogame\win32_herogame.cpp user32.lib Gdi32.lib
popd
