
@echo off
SetLocal EnableDelayedExpansion
cls

set sdl_include=W:\dev\libs\SDL2_VS\include
set src_dir=W:\dev\c++\ld_37_codebase\src

set libs=SDL2main.lib SDL2.lib
set source_files=%src_dir%\ld_37.cpp
set macros=
set compiler_flags=/Zi /MD 
set linker_flags=/SUBSYSTEM:WINDOWS
set executable=ld_37.exe

pushd bin

cl %compiler_flags% /Fe%executable% /I%sdl_include% %macros% %source_files% %libs% /link %linker_flags%

popd