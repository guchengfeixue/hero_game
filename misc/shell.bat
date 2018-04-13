@echo off

REM
REM  To run this at startup, use this as your shortcut target:
REM  %windir%\system32\cmd.exe /k F:\project\github\hero_game\misc\shell.bat
REM

if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build" call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build" if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC" call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
if not exist w: if exist C:\github\hero_game subst w: C:\github\hero_game
if not exist w: if exist F:\project\github\hero_game subst w: F:\project\github\hero_game
set path=w;%path%
w:
cd code

