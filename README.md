# Chrome Dino

A clone of the Dinosaur Game from Google Chrome. Press `SPACE` to jump and `DOWN` to duck.

<br />

<p align="center">
    <img src="https://github.com/AlexEidt/docs/blob/master/Dino/dino.gif" alt="Dinosaur Game" />
</p>

<br />

## Compilation

See lines `73-126` in `olcPixelGameEngine.h` for more details.

### Windows

```
g++ -o game.exe game.cpp -luser32 -lgdi32 -lopengl32 -lgdiplus -lShlwapi -ldwmapi -lstdc++fs -lwinmm -static -std=c++17
```

### Linux

```
g++ -o game game.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17
```

### Mac

```
clang++ -arch x86_64 -std=c++17 -mmacosx-version-min=10.15 -Wall -framework OpenGL 
		-framework GLUT -framework Carbon -lpng game.cpp -o game
```