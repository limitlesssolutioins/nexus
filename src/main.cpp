#include "Game.h"

int main(int argc, char* argv[]) {
    // Configuración de Ventana
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;
    const int FPS = 60;
    const int FRAME_DELAY = 1000 / FPS;

    Game* nexusGame = new Game();

    if (nexusGame->init("Nexus: World - Alpha Build", SCREEN_WIDTH, SCREEN_HEIGHT)) {
        
        Uint32 frameStart;
        int frameTime;

        // --- GAME LOOP ---
        while (nexusGame->isRunning()) {
            frameStart = SDL_GetTicks();

            // 1. Input
            nexusGame->handleEvents();

            // 2. Update (DeltaTime simulado por ahora)
            // En el futuro calcularemos DT real, por ahora asumimos paso fijo
            nexusGame->update(0.016f); 

            // 3. Render
            nexusGame->render();

            // Control de FPS
            frameTime = SDL_GetTicks() - frameStart;
            if (FRAME_DELAY > frameTime) {
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    }

    nexusGame->clean();
    delete nexusGame;
    return 0;
}