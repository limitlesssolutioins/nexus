# Echoes of the Rift - Project Status & Documentation

## 1. Vision General
**Echoes of the Rift** es un Action RPG top-down desarrollado en C++ y Raylib. Inspirado en Hunter x Hunter y Solo Leveling, el juego se centra en la exploración de mazmorras procedurales, un sistema de clases libre y combate táctico basado en el equipamiento.

## 2. Pilares del Juego
*   **Exploración Continua:** Dungeons procedurales extensos sin barreras artificiales. El progreso es geográfico: avanza hasta encontrar al Boss.
*   **Combate por Armas:** El ataque básico cambia drásticamente según el arma (Arcos de golpe melee, proyectiles físicos o mágicos).
*   **Sistema de Calidad:** Los ítems se definen por Rango (F a S) y Calidad (0-100%). Un ítem Rango E "Exquisito" puede ser superior a uno Rango D "Dañado".
*   **Sigilo y Percepción:** Los enemigos usan Raycasting para la visión y detección por sonido. Es posible flanquear y evitar combates usando el entorno.
*   **Progresion Persistente:** Nivel, stats y equipo se mantienen. Hunter Rank desbloquea rifts más profundos.

## 3. Progreso Actual

### Implementado y Funcional (Core Gameplay)
- [x] **Dungeon Generator:** Generación de Cuevas Orgánicas (Cellular Automata) y Ruinas Estructuradas (BSP/Rooms) según el tema del Rift.
- [x] **Conectividad Garantizada:** Algoritmo "Excavator" que asegura un camino transitable desde el inicio hasta el Boss.
- [x] **IA de Percepción:** Enemigos con cono de visión limitado por muros (Raycasting) y audición circular (atraviesa muros).
- [x] **Física Avanzada:** Wall Sliding para todas las entidades (jugador, enemigos e invocaciones).
- [x] **Ataque por Armas:**
    - *Espada/Daga/Maza:* Ataques de área (Melee Arcs) con knockback y escalado físico.
    - *Arco:* Proyectiles físicos de alta velocidad.
    - *Bastón/Orbe:* Proyectiles mágicos con escalado de inteligencia.
- [x] **Sistema de Calidad de Items:** Generación de loot con prefijos dinámicos (Damaged, Fine, Exquisite, Masterwork) que afectan los stats base.
- [x] **Invocaciones Mejoradas:** IA de ataque continuo, manejo de colisiones físicas y sistema de aggro (los enemigos priorizan al objetivo más cercano).
- [x] **HUD de Jefe:** Barra de vida dinámica para el Boss del dungeon.
- [x] **Persistencia:** Save/load binario (SAVE_VERSION=3).

### En Progreso
- [ ] **Boss patterns:** Los jefes necesitan ataques especiales y fases (actualmente usan IA de combate básico mejorada).
- [ ] **Variedad de Enemigos:** Implementar habilidades únicas para Spitters (distancia) y Phantoms (stealth).

## 4. Arquitectura del Codigo

| Archivo | Responsabilidad |
|---------|-----------------|
| `src/types.h` | Estructuras base, Enums y constantes de balance. |
| `src/main.cpp` | Game loop, Input de combate por arma, Cámara dinámica "Look-ahead". |
| `src/combat.cpp` | IA con Raycasting, Colisiones (Wall Slide), PerformMeleeAttack, Lógica de Invocaciones. |
| `src/progression.cpp` | Recálculo de stats, fórmulas de daño/defensa, tabla de 42 habilidades. |
| `src/drops.cpp` | Generador de ítems Rango+Calidad, lógica de pickups. |
| `src/rift.cpp` | Generador procedural de Dungeons (Organic vs Structured), Populador de enemigos. |
| `src/ui.cpp` | HUD con Boss Bar, Menús de inventario y Skill Tree. |

## 5. Sistemas de Juego

### 5.1 Stats y Calidad de Ítems
Un ítem ahora tiene un valor de **Quality (0.0 a 1.0)**.
*   **Fórmula de Ataque:** `Base * RankMult * (0.8 + Quality * 0.4)`
*   **Prefijos de Calidad:**
    *   0.0 - 0.2: *Damaged* (Mult 0.8x)
    *   0.2 - 0.4: *Rusty/Old*
    *   0.6 - 0.8: *Fine*
    *   0.8 - 0.95: *Exquisite*
    *   0.95 - 1.0: *Masterwork* (Mult 1.2x)

### 5.2 Tipos de Ataque Básico (LMB)
| Arma | Tipo | Características |
|------|------|-----------------|
| Sword | Melee | Arco 120°, Daño 120%, Knockback medio. |
| Dagger | Melee | Arco 60°, Daño 70%, Muy rápida. |
| Mace | Melee | Arco 90°, Daño 150%, Knockback masivo. |
| Bow | Ranged | Proyectil físico, escala con STR/DEX. |
| Staff | Ranged | Proyectil mágico, escala con INT. |

## 6. Referencia Tecnica

### Fórmulas de Combate
*   **Daño Recibido:** `(Daño_Enemigo - Defensa_Plana_Equipo) * (1.0 - Reducción_Porcentual_END)`
*   **Visión IA:** Cono de 60° con Raycast contra `vector<Obstacle>`.
*   **Nivel de Enemigos:** Escala según la distancia (X) desde el punto de inicio del dungeon.

### Notas de Desarrollo
- El archivo de save es `savegame.dat`. Al cambiar la estructura de `Equipment`, se ha subido la versión a `3`. Versiones anteriores serán ignoradas para evitar crashes.
- El mapa tiene un tamaño lógico de `DUNGEON_WIDTH * TILE_SIZE` (aprox 4000px). El jugador ya no está confinado a la pantalla de 720p de alto.

## 7. Controles
| Tecla | Accion |
|-------|--------|
| WASD | Movimiento |
| Click Izquierdo | Ataque de Arma (Melee o Proyectil) |
| 1-6 | Habilidades equipadas |
| TAB | Skill Tree |
| C | Hunter Status |
| I | Inventario |
| ENTER | Iniciar Rift / Volver al Hub |
