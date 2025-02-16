@echo off
:: .
:: .
:: > Setup required Environment
:: -------------------------------------
set INCLUDE_DIR="include"
set LIB_DIR="lib"
set COMPILER_DIR=C:\raylib\w64devkit\bin
set PATH=%PATH%;%COMPILER_DIR%
:: .
:: > Cleaning latest build
:: ---------------------------
cmd /c if exist main.exe del /F main.exe
:: .
:: > Compiling program
:: --------------------------
g++ -o main.exe lib/imgui.o lib/jsoncpp.o ./src/*.cpp -mconsole -v -s -O3 -I%INCLUDE_DIR% -L%LIB_DIR% -lmingw32 -lSDL2main -lSDL2 -lkernel32 -lwinmm -lgdi32
:: .
:: > Executing program
:: -------------------------
cmd /c if exist main.exe main.exe
