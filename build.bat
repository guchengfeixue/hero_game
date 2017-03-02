@echo off
if not exist ..\cmd_build mkdir ..\cmd_build
pushd ..\cmd_build
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHEROGAME_INTERNAL=1 -DHEROGAME_SLOW=1 -DHEROGAME_WIN32=1 -FC -Z7 -Fmwin32_herogame.map ..\herogame\win32_herogame.cpp /link -opt:ref user32.lib Gdi32.lib
popd
