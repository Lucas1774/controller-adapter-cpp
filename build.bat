@echo off
if not exist build mkdir build

echo Compiling in parallel...

start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -ISDL2-2.30.6/x86_64-w64-mingw32/include framework/funcs.cpp -o build/funcs.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include framework/configParser.cpp -o build/configParser.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Iframework -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include swarm/swarm.cpp -o build/swarm.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Iframework -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include tft/tft.cpp -o build/tft.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Iframework -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include chess/chess.cpp -o build/chess.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Iframework -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include frostpunkTwo/frostpunkTwo.cpp -o build/frostpunkTwo.o"
start "" /B cmd /C "g++ -c -fdiagnostics-color=always -g -Iframework -Iswarm -Itft -Ichess -IfrostpunkTwo -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include main.cpp -o build/main.o"

echo Waiting for compilation to finish...
:waitloop
tasklist /FI "IMAGENAME eq g++.exe" | find /I "g++.exe" >nul 2>&1
if %ERRORLEVEL%==0 (
    timeout /t 1 >nul
    goto waitloop
)

echo Linking...
g++ -fdiagnostics-color=always -g build/funcs.o build/configParser.o build/swarm.o build/tft.o build/chess.o build/frostpunkTwo.o build/main.o -o mapper.exe -Ljsoncpp-1.9.5/lib -LSDL2-2.30.6/x86_64-w64-mingw32/lib -ljsoncpp -lSDL2main -lSDL2

echo Build complete.
