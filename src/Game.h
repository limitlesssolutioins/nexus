#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <iostream>
#include "types.h"

class Game {
public:
    Game();
    ~Game();

    bool init(const char* title, int width, int height);
    void handleEvents();
    void update(float deltaTime);
    void render();
    void clean();
    bool isRunning() { return running; }

private:
    bool running;
    SDL_Window* window;
    SDL_Renderer* renderer;

    // --- Entidades del Mundo (Prototipo) ---
    Vector2D playerPos;      // Posición absoluta en el mundo
    Vector2D playerVelocity; // Velocidad actual
    float playerSpeed;       // Velocidad base
    Rect camera;             // Rectángulo de la cámara (Viewport)

    // Mapa (temporal para referencia visual)
    int mapWidth;
    int mapHeight;
};

#endif // GAME_H
