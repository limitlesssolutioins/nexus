#include "Game.h"

Game::Game() : running(false), window(nullptr), renderer(nullptr), playerSpeed(300.0f) {
    // Inicializar posición del jugador en el centro de un mapa teórico grande
    playerPos = Vector2D(1000.0f, 1000.0f);
    mapWidth = 4000;
    mapHeight = 4000;
}

Game::~Game() {}

bool Game::init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL Init Falló: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cout << "Crear Ventana Falló: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Crear Renderer Falló: " << SDL_GetError() << std::endl;
        return false;
    }

    // Inicializar cámara
    camera = {0, 0, (float)width, (float)height};
    running = true;
    
    std::cout << "Sistema Nexus Iniciado. Motor Gráfico: SDL2." << std::endl;
    return true;
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
    }

    // --- INPUT CONTINUO (Estilo WASD Acción) ---
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
    
    playerVelocity.x = 0;
    playerVelocity.y = 0;

    if (currentKeyStates[SDL_SCANCODE_W]) playerVelocity.y = -1;
    if (currentKeyStates[SDL_SCANCODE_S]) playerVelocity.y = 1;
    if (currentKeyStates[SDL_SCANCODE_A]) playerVelocity.x = -1;
    if (currentKeyStates[SDL_SCANCODE_D]) playerVelocity.x = 1;

    // Normalizar para que diagonal no sea más rápido
    playerVelocity.normalize();
}

void Game::update(float deltaTime) {
    // 1. Mover Jugador
    playerPos = playerPos + (playerVelocity * playerSpeed * deltaTime);

    // 2. Límites del Mundo (World Bounds)
    if (playerPos.x < 0) playerPos.x = 0;
    if (playerPos.y < 0) playerPos.y = 0;
    if (playerPos.x > mapWidth) playerPos.x = mapWidth;
    if (playerPos.y > mapHeight) playerPos.y = mapHeight;

    // 3. Actualizar Cámara (Centrada en el jugador)
    camera.x = playerPos.x - (camera.w / 2);
    camera.y = playerPos.y - (camera.h / 2);

    // Clamp de la cámara a los bordes del mapa
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
    if (camera.x > mapWidth - camera.w) camera.x = mapWidth - camera.w;
    if (camera.y > mapHeight - camera.h) camera.y = mapHeight - camera.h;
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255); // Fondo Gris Oscuro (Nexus Void)
    SDL_RenderClear(renderer);

    // --- DIBUJAR MUNDO (GRID) ---
    // Dibujamos una cuadrícula para tener referencia de movimiento
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    int gridSize = 100;
    
    // Solo dibujar las líneas que están visibles en la cámara (Optimización básica)
    int startCol = (int)(camera.x / gridSize);
    int endCol = startCol + (camera.w / gridSize) + 1;
    int startRow = (int)(camera.y / gridSize);
    int endRow = startRow + (camera.h / gridSize) + 1;

    for (int x = startCol; x <= endCol; x++) {
        for (int y = startRow; y <= endRow; y++) {
            SDL_Rect tileRect;
            tileRect.x = (x * gridSize) - (int)camera.x;
            tileRect.y = (y * gridSize) - (int)camera.y;
            tileRect.w = gridSize;
            tileRect.h = gridSize;
            SDL_RenderDrawRect(renderer, &tileRect);
        }
    }

    // --- DIBUJAR JUGADOR ---
    SDL_Rect playerRect;
    playerRect.w = 32;
    playerRect.h = 32;
    // Conversión: Mundo -> Pantalla
    playerRect.x = (int)(playerPos.x - camera.x - (playerRect.w / 2)); 
    playerRect.y = (int)(playerPos.y - camera.y - (playerRect.h / 2));

    // Color del Protagonista (Cyan Brillante - Energía de Aura)
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); 
    SDL_RenderFillRect(renderer, &playerRect);

    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    std::cout << "Nexus Cerrado Correctamente." << std::endl;
}
