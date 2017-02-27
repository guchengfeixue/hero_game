@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
subst w: C:\github\hero_game
set path=w;%path%
w:


