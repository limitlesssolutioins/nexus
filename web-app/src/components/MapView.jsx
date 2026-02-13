import { useCallback, useEffect, useRef, useState } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import markerIcon2x from 'leaflet/dist/images/marker-icon-2x.png';
import markerIcon from 'leaflet/dist/images/marker-icon.png';
import markerShadow from 'leaflet/dist/images/marker-shadow.png';

import mapData from '../map_data.json';
import worldMapImage from '../assets/Nexus.png';
import valdoriaMapImage from '../assets/valdoria.png';
import arkhosMapImage from '../assets/arkhos.png';
import umbaraMapImage from '../assets/umbara.png';
import veranthosMapImage from '../assets/veranthos.png';
import severynMapImage from '../assets/severyn.png';
import kaijinMapImage from '../assets/kaijin.png';
import alRashidMapImage from '../assets/al-rashid.png';
import marcasMapImage from '../assets/marcas.png';
import aegisMapImage from '../assets/aegis.png';

const regionalImageMap = {
  valdoria: valdoriaMapImage,
  arkhos: arkhosMapImage,
  umbara: umbaraMapImage,
  veranthos: veranthosMapImage,
  severyn: severynMapImage,
  kaijin: kaijinMapImage,
  'al-rashid': alRashidMapImage,
  'marcas-destrozadas': marcasMapImage,
  aegis: aegisMapImage,
};

delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: markerIcon2x,
  iconUrl: markerIcon,
  shadowUrl: markerShadow,
});

export default function MapView({ onSelectSector }) {
  const worldMapRef = useRef(null);
  const regionMapRef = useRef(null);
  const [currentView, setCurrentView] = useState('world');

  const worldInstance = useRef(null);
  const regionInstance = useRef(null);
  const regionMarkersLayer = useRef(L.layerGroup());
  const worldSectorsLayer = useRef(L.layerGroup());

  const handleSectorClick = useCallback((sectorId) => {
    const sector = mapData.worldMap.sectors.find((item) => item.id === sectorId);
    if (!sector || !regionalImageMap[sectorId]) return;

    const polygon = L.polygon(sector.coords);
    worldInstance.current?.flyToBounds(polygon.getBounds(), { duration: 1.1 });
    onSelectSector?.(sectorId);
    setTimeout(() => setCurrentView(sectorId), 1100);
  }, [onSelectSector]);

  useEffect(() => {
    if (worldMapRef.current && !worldInstance.current) {
      const worldMapData = mapData.worldMap;
      worldInstance.current = L.map(worldMapRef.current, {
        crs: L.CRS.Simple,
        minZoom: -1,
        maxZoom: 2,
        attributionControl: false,
      });
      const worldBounds = [[0, 0], [worldMapData.height, worldMapData.width]];
      L.imageOverlay(worldMapImage, worldBounds).addTo(worldInstance.current);
      worldSectorsLayer.current.addTo(worldInstance.current);
      worldInstance.current.fitBounds(worldBounds);

      worldMapData.sectors.forEach((sector) => {
        const polygon = L.polygon(sector.coords, {
          color: 'rgba(227, 220, 201, 0.9)',
          weight: 1.5,
          fillOpacity: 0.12,
        }).addTo(worldSectorsLayer.current);
        polygon.on('mouseover', function onHover() {
          this.setStyle({ fillOpacity: 0.22, weight: 2.5 });
        });
        polygon.on('mouseout', function onOut() {
          this.setStyle({ fillOpacity: 0.12, weight: 1.5 });
        });
        polygon.on('click', (event) => {
          L.DomEvent.stop(event);
          handleSectorClick(sector.id);
        });
      });
    }

    if (regionMapRef.current && !regionInstance.current) {
      regionInstance.current = L.map(regionMapRef.current, {
        crs: L.CRS.Simple,
        minZoom: -1,
        maxZoom: 3,
        attributionControl: false,
      });
      regionMarkersLayer.current.addTo(regionInstance.current);
    }

    return () => {
      if (worldInstance.current) worldInstance.current.remove();
      if (regionInstance.current) regionInstance.current.remove();
    };
  }, [handleSectorClick]);

  const handleBackToWorld = () => {
    setCurrentView('world');
    const worldBounds = [[0, 0], [mapData.worldMap.height, mapData.worldMap.width]];
    worldInstance.current?.fitBounds(worldBounds, { animate: true, duration: 0.7 });
  };

  useEffect(() => {
    if (!worldMapRef.current || !regionMapRef.current) return;
    if (currentView === 'world') {
      worldMapRef.current.classList.add('active');
      regionMapRef.current.classList.remove('active');
      return;
    }
    worldMapRef.current.classList.remove('active');
    regionMapRef.current.classList.add('active');
    loadRegionMap(currentView);
  }, [currentView]);

  const loadRegionMap = (regionId) => {
    const map = regionInstance.current;
    if (!map) return;

    const regionData = mapData.regionalMaps[regionId];
    const regionImage = regionalImageMap[regionId];
    if (!regionData || !regionImage) return;

    const bounds = [[0, 0], [regionData.height, regionData.width]];
    regionMarkersLayer.current.clearLayers();
    map.eachLayer((layer) => {
      if (layer instanceof L.ImageOverlay) map.removeLayer(layer);
    });

    L.imageOverlay(regionImage, bounds).addTo(map);
    map.fitBounds(bounds, { animate: false });

    if (regionData.locaciones) {
      regionData.locaciones.forEach((loc) => {
        const marker = L.marker([loc.coords.y, loc.coords.x]);
        marker.bindPopup(`<b>${loc.nombre}</b><br>${loc.descripcion}`);
        regionMarkersLayer.current.addLayer(marker);
      });
    }
  };

  return (
    <div className="map-view">
      <div ref={worldMapRef} className="map-canvas active" />
      <div ref={regionMapRef} className="map-canvas" />
      {currentView !== 'world' && (
        <button type="button" onClick={handleBackToWorld} className="map-back-button">
          Volver al mapa global
        </button>
      )}
    </div>
  );
}
