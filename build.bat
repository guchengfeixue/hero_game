@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -wd4505 -DHEROGAME_INTERNAL=1 -DHEROGAME_SLOW=1 -DHEROGAME_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib

REM TODO - can we just build both with one exe?

if not exist ..\cmd_build mkdir ..\cmd_build
pushd ..\cmd_build

REM 32-bit build
REM cl  %CommonCompilerFlag% ..\herogame\win32_herogame.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
del *.pdb > NUL 2>NUL

set hr=%time:~0,2%
set hr=%hr: =0%
REM Lletter replace, space relaced by 0 
REM cl %CommonCompilerFlags% ..\herogame\herogame.cpp  -Fmherogame.map /LD /link -incremental:no -PDB:herogame_%date:~0,4%_%date:~5,2%_%date:~8,2%_%hr%_%time:~3,2%_%time:~6,2%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\herogame\herogame.cpp  -Fmherogame.map /LD /link -incremental:no -PDB:herogame_%random%.pdb -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\herogame\win32_herogame.cpp  -Fmwin32_herogame.map /link %CommonLinkerFlags%

popd
