@echo off
if not exist build mkdir build

echo Compiling in parallel...

start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -ISDL2-2.30.6/include framework/funcs.cpp -o build/funcs.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -ISDL2-2.30.6/include  -Ijsoncpp-1.9.5/include -Iframework include/configParser.cpp -o build/configParser.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -ISDL2-2.30.6/include  -Ijsoncpp-1.9.5/include -Iframework include/gameRegistry.cpp -o build/gameRegistry.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude swarm/swarm.cpp -o build/swarm.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude tft/tft.cpp -o build/tft.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude chess/chess.cpp -o build/chess.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude frostpunkTwo/frostpunkTwo.cpp -o build/frostpunkTwo.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude megabonk/megabonk.cpp -o build/megabonk.o"
start "" /B cmd /C "g++ -c -std=c++20 -fdiagnostics-color=always -g -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/include -Iframework -Iinclude main.cpp -o build/main.o"

echo Waiting for compilation to finish...
:waitloop
tasklist /FI "IMAGENAME eq g++.exe" | find /I "g++.exe" >nul 2>&1
if %ERRORLEVEL%==0 (
    timeout /t 1 >nul
    goto waitloop
)

echo Linking...
g++ -std=c++20 -fdiagnostics-color=always -g build/funcs.o build/configParser.o build/gameRegistry.o build/swarm.o build/tft.o build/chess.o build/frostpunkTwo.o build/megabonk.o build/main.o -o mapper.exe -Ljsoncpp-1.9.5/lib -LSDL2-2.30.6/lib -ljsoncpp -lSDL2main -lSDL2

echo Build complete.
