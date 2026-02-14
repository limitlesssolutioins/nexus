# Nexus: Game Design Document (GDD)

## 1. Visión General
**Género:** Open World Action RPG con Gestión de Gremio.
**Perspectiva:** Top-Down / Isométrica (2.5D).
**Estilo:** "World of Warcraft meets Hades". Exploración libre, combate de acción rápido, gestión estratégica a largo plazo.
**Motor:** C++ Custom Engine (SDL2 + ECS).

## 2. Pilares de Jugabilidad

### A. El Mundo Abierto (The Living Map)
El juego no ocurre en niveles aislados, sino en un mapa continuo dividido en regiones.
- **Ciudades:** Zonas seguras con NPCs, comercio, y política.
- **Wildlands:** Zonas salvajes donde aparecen Rifts procedimentales.
- **Ciclo Día/Noche:** Afecta el tipo de enemigos y la aparición de Rifts (más peligrosos de noche).

### B. Sistema de Rifts Dinámicos
Los Rifts no son estáticos. Un sistema de "Director de IA" decide dónde aparecen basándose en:
- **Región:** Rifts de Fuego en zonas volcánicas/industriales, Rifts de Vacío en zonas oscuras.
- **Amenaza:** Si el jugador ignora un Rift pequeño mucho tiempo, este crece y empieza a corromper el mapa (spawnear monstruos fuera del Rift).
- **Entrada:** Al entrar en un Rift, el juego carga una "Instancia" (Dungeon Procedimental). Al salir, vuelves al Mapa Abierto.

### C. Gestión de Gremio (The Enterprise)
El jugador funda y dirige su propia organización de cazadores.
- **Renombre:** Al cerrar Rifts, ganas fama. Esto te da acceso a mejores contratos de las Facciones (Valdoria, Arkhos, etc.).
- **Reclutamiento:** Contrata NPCs con stats únicos para que protejan zonas o te acompañen.
- **Economía:** El mercado de Cristales fluctúa. Vende cuando la demanda es alta (guerra) para financiar tu equipo.

## 3. Arquitectura Técnica (C++)

### Core Systems
1.  **World Streaming:** Carga/Descarga de "Chunks" del mapa para permitir un mundo grande sin consumir toda la RAM.
2.  **Entity Component System (ECS):** Vital para manejar cientos de entidades (NPCs, Arboles, Rifts, Items) simultáneamente.
3.  **Event Bus:** Sistema para que el "Mundo" reaccione al "Jugador" (ej: Matas un boss -> La reputación con Valdoria sube -> Los precios en la tienda bajan).

### Estructura de Datos (Save File)
El estado del mundo debe persistir.
- ¿Qué Rifts están abiertos?
- ¿Qué edificios del Gremio están construidos?
- ¿En qué año estamos (Timeline de 20 años)?

## 4. Progresión (The 20-Year Arc)
El juego se divide en 3 Eras Tecnológicas que cambian el gameplay:
- **Era 1 (Años 1-5):** Aventura. Exploración a pie. Rifts simples.
- **Era 2 (Años 6-15):** Guerra. Uso de vehículos/monturas. Rifts de combate masivo. Gestión de tropas.
- **Era 3 (Años 16-20):** Supervivencia. El mapa es hostil (radiación). Tecnología avanzada pero escasa.

## 5. Roadmap de Desarrollo
1.  **Prototipo de Movimiento en Mundo Abierto:** Mover al PJ en un mapa grande con cámara de seguimiento.
2.  **Sistema de Spawns:** Hacer que aparezca un "objeto" (Rift) en coordenadas aleatorias.
3.  **Transición Mundo <-> Instancia:** Entrar al Rift y cargar el mapa de combate.
4.  **Sistema de Gremio UI:** Menús básicos de gestión.
