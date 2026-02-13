import React, { useEffect, useRef, useState } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import './Map.css';

// 1. Importar todos los assets al principio
import mapData from './map_data.json';
import worldMapImage from './assets/Nexus.png';
import valdoriaMapImage from './assets/valdoria.png';
import arkhosMapImage from './assets/arkhos.png';
import umbaraMapImage from './assets/umbara.png';
import veranthosMapImage from './assets/veranthos.png';
import severynMapImage from './assets/severyn.png';
import kaijinMapImage from './assets/kaijin.png';
import alRashidMapImage from './assets/al-rashid.png';
import marcasMapImage from './assets/marcas.png';
import aegisMapImage from './assets/aegis.png';


// 2. Crear un mapeo de IDs a las imágenes importadas
const regionalImageMap = {
    valdoria: valdoriaMapImage,
    arkhos: arkhosMapImage,
    umbara: umbaraMapImage,
    veranthos: veranthosMapImage,
    severyn: severynMapImage,
    kaijin: kaijinMapImage,
    'al-rashid': alRashidMapImage,
    'marcas-destrozadas': marcasMapImage,
    aegis: aegisMapImage
};

// Ajustar iconos por defecto de Leaflet
delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
    iconRetinaUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon-2x.png',
    iconUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-icon.png',
    shadowUrl: 'https://unpkg.com/leaflet@1.9.4/dist/images/marker-shadow.png',
});

const Map = () => {
    const worldMapRef = useRef(null);
    const regionMapRef = useRef(null);
    const [currentView, setCurrentView] = useState('world');

    const mapWorldInstance = useRef(null);
    const mapRegionInstance = useRef(null);
    const regionMarkersLayer = useRef(L.layerGroup());

    // --- EFECTO PRINCIPAL DE INICIALIZACIÓN ---
    useEffect(() => {
        
        // Inicializar Mapa Mundial
        if (worldMapRef.current && !mapWorldInstance.current) {
            const worldMapData = mapData.worldMap;
            mapWorldInstance.current = L.map(worldMapRef.current, { crs: L.CRS.Simple, minZoom: -1, maxZoom: 2, attributionControl: false });
            const worldBounds = [[0, 0], [worldMapData.height, worldMapData.width]];
            L.imageOverlay(worldMapImage, worldBounds).addTo(mapWorldInstance.current);
            mapWorldInstance.current.fitBounds(worldBounds);

            // Herramienta de coordenadas (solo en consola)
            mapWorldInstance.current.on('click', (e) => {
                const latLng = e.latlng;
                const coordString = `[${Math.round(latLng.lat)}, ${Math.round(latLng.lng)}]`;
                console.log(`%cCoordenadas del clic -> ${coordString}`, 'color: #00ff00; font-size: 1.2em; font-weight: bold;');
            });

            // Crear polígonos de sectores (incluyendo Aegis ahora)
            worldMapData.sectors.forEach(sector => {
                const polygon = L.polygon(sector.coords, { color: 'rgba(255,255,255,0.5)', weight: 2, fillOpacity: 0.1 }).addTo(mapWorldInstance.current);
                polygon.on('mouseover', function () { this.setStyle({ fillOpacity: 0.3, weight: 3 }); });
                polygon.on('mouseout', function () { this.setStyle({ fillOpacity: 0.1, weight: 2 }); });
                polygon.on('click', (e) => {
                    L.DomEvent.stop(e);
                    handleSectorClick(sector.id);
                });
            });
        }

        // Inicializar Contenedor de Mapa Regional
        if (regionMapRef.current && !mapRegionInstance.current) {
            mapRegionInstance.current = L.map(regionMapRef.current, { crs: L.CRS.Simple, minZoom: -1, maxZoom: 3, attributionControl: false });
            regionMarkersLayer.current.addTo(mapRegionInstance.current);
        }

        return () => {
            if (mapWorldInstance.current) mapWorldInstance.current.remove();
            if (mapRegionInstance.current) mapRegionInstance.current.remove();
        };
    }, []);

    const handleSectorClick = (sectorId) => {
        const sector = mapData.worldMap.sectors.find(s => s.id === sectorId);
        if (!sector || !regionalImageMap[sectorId]) return;

        const polygon = L.polygon(sector.coords);
        mapWorldInstance.current.flyToBounds(polygon.getBounds(), { duration: 1.5 });

        setTimeout(() => setCurrentView(sectorId), 1500);
    };

    const handleBackToWorld = () => {
        setCurrentView('world');
        const worldBounds = [[0, 0], [mapData.worldMap.height, mapData.worldMap.width]];
        mapWorldInstance.current.fitBounds(worldBounds, { animate: true, duration: 1 });
    };

    // --- EFECTO PARA ACTUALIZAR VISTA ---
    useEffect(() => {
        if (currentView === 'world') {
            worldMapRef.current.classList.add('active');
            regionMapRef.current.classList.remove('active');
        } else {
            worldMapRef.current.classList.remove('active');
            regionMapRef.current.classList.add('active');
            loadRegionMap(currentView);
        }
    }, [currentView]);

    const loadRegionMap = (regionId) => {
        const map = mapRegionInstance.current;
        if (!map) return;

        const regionData = mapData.regionalMaps[regionId];
        const regionImage = regionalImageMap[regionId];
        const bounds = [[0, 0], [regionData.height, regionData.width]];
        
        regionMarkersLayer.current.clearLayers();
        map.eachLayer(layer => {
            if (layer instanceof L.ImageOverlay) map.removeLayer(layer);
        });

        L.imageOverlay(regionImage, bounds).addTo(map);
        map.fitBounds(bounds, { animate: false });

        if (regionData.locaciones) {
            regionData.locaciones.forEach(loc => {
                const marker = L.marker([loc.coords.y, loc.coords.x]);
                marker.bindPopup(`<b>${loc.nombre}</b><br>${loc.descripcion}`);
                regionMarkersLayer.current.addLayer(marker);
            });
        }
    };

    return (
        <>
            <div ref={worldMapRef} id="map-world" className="map-container active"></div>
            <div ref={regionMapRef} id="map-region" className="map-container"></div>
            {currentView !== 'world' && (
                <button onClick={handleBackToWorld} className="back-button">Volver a Nexus Prime</button>
            )}
        </>
    );
};

export default Map;
