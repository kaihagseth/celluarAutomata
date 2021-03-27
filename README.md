# celluarAutomata
Celluar automata using convolutions
Dependencies: 
Intel TBB

To build:
g++ -o testProg pixelTest.cpp -lX11 -lGL -lpthread -ltbb -lpng -lstdc++fs -std=c++17 -O3 -I $TBBROOT/include --fast-math

This project uses OLC pixel game engine!
https://github.com/OneLoneCoder/olcPixelGameEngine
