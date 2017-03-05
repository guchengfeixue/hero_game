@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHEROGAME_INTERNAL=1 -DHEROGAME_SLOW=1 -DHEROGAME_WIN32=1 -FC -Z7 -Fmwin32_herogame.map
set CommonLinkerFlags=-opt:ref user32.lib Gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

if not exist ..\cmd_build mkdir ..\cmd_build
pushd ..\cmd_build

REM 32-bit build
REM cl  %CommonCompilerFlag% ..\herogame\win32_herogame.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
cl %CommonCompilerFlags% ..\herogame\win32_herogame.cpp /link %CommonLinkerFlags%

popd
