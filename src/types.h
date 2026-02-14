#ifndef TYPES_H
#define TYPES_H

#include <cmath>

// Estructura para manejar coordenadas y velocidades flotantes (Movimiento suave)
struct Vector2D {
    float x;
    float y;

    Vector2D() : x(0), y(0) {}
    Vector2D(float x, float y) : x(x), y(y) {}

    // Sobrecarga de operadores para facilitar la física
    Vector2D operator+(const Vector2D& v) const { return Vector2D(x + v.x, y + v.y); }
    Vector2D operator-(const Vector2D& v) const { return Vector2D(x - v.x, y - v.y); }
    Vector2D operator*(float s) const { return Vector2D(x * s, y * s); }
    
    // Normalizar vector (para que moverse en diagonal no sea más rápido)
    void normalize() {
        float mag = std::sqrt(x * x + y * y);
        if (mag > 0) {
            x /= mag;
            y /= mag;
        }
    }
};

// Rectángulo simple para colisiones y cámara
struct Rect {
    float x, y, w, h;
};

// Estados del Juego (para el futuro)
enum class GameState {
    MainMenu,
    Exploration, // Mundo Abierto
    Combat,      // Posiblemente instanciado o en mundo
    GuildManagement, // UI de Base
    Inventory
};

#endif // TYPES_H