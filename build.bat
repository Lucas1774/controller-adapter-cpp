@echo off
if not exist build mkdir build
g++ -c -fdiagnostics-color=always -g -Iutil -Iswarm -Itft -Ichess -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include util/funcs.cpp -o build/funcs.o
g++ -c -fdiagnostics-color=always -g -Iutil -Iswarm -Itft -Ichess -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include swarm/swarm.cpp -o build/swarm.o
g++ -c -fdiagnostics-color=always -g -Iutil -Iswarm -Itft -Ichess -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include tft/tft.cpp -o build/tft.o
g++ -c -fdiagnostics-color=always -g -Iutil -Iswarm -Itft -Ichess -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include chess/chess.cpp -o build/chess.o
g++ -c -fdiagnostics-color=always -g -Iutil -Iswarm -Itft -Ichess -Ijsoncpp-1.9.5/include -ISDL2-2.30.6/x86_64-w64-mingw32/include main.cpp -o build/main.o
g++ -fdiagnostics-color=always -g build/funcs.o build/swarm.o build/tft.o build/chess.o build/main.o -o mapper.exe -Ljsoncpp-1.9.5/lib -LSDL2-2.30.6/x86_64-w64-mingw32/lib -ljsoncpp -lSDL2main -lSDL2
