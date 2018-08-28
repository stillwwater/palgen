@echo off

mkdir build
pushd build
cl -Zi /I "..\src" ..\src\palgen.c ..\src\parser.c  user32.lib gdi32.lib /LINK
popd
