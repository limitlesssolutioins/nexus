import storyData from '../content/generated/storybible.json';
import mapData from '../map_data.json';

export const STORY = storyData;
export const MAP_DATA = mapData;

const TYPE_LABELS = {
  chapter: 'Capitulo',
  character: 'Personaje',
  faction: 'Faccion',
  enemy: 'Enemigo',
  lore: 'Lore',
  world: 'Mundo',
  reference: 'Referencia',
  template: 'Plantilla',
};

function normalize(value) {
  return (value || '')
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .toLowerCase()
    .trim();
}

export function getTypeLabel(type) {
  return TYPE_LABELS[type] || 'Otro';
}

export function buildEntryIndex(entries) {
  const byId = new Map(entries.map((entry) => [entry.id, entry]));
  const byTitle = new Map(entries.map((entry) => [normalize(entry.title), entry]));
  return { byId, byTitle };
}

export function searchEntries(entries, query) {
  const q = normalize(query);
  if (!q) return entries;
  return entries.filter((entry) => {
    const haystack = [
      entry.title,
      entry.type,
      entry.path,
      ...(entry.tags || []),
      entry.content.slice(0, 400),
    ]
      .join(' ')
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .toLowerCase();
    return haystack.includes(q);
  });
}

export function groupByType(entries) {
  return entries.reduce((acc, entry) => {
    if (!acc[entry.type]) acc[entry.type] = [];
    acc[entry.type].push(entry);
    return acc;
  }, {});
}

export function buildGraph(relations) {
  const outgoing = new Map();
  const incoming = new Map();
  for (const rel of relations) {
    if (!outgoing.has(rel.from)) outgoing.set(rel.from, []);
    if (!incoming.has(rel.to)) incoming.set(rel.to, []);
    outgoing.get(rel.from).push(rel);
    incoming.get(rel.to).push(rel);
  }
  return { outgoing, incoming };
}

const sectorToEntryKey = {
  aegis: 'aegis',
  valdoria: 'valdoria',
  arkhos: 'arkhos',
  umbara: 'umbara',
  veranthos: 'veranthos',
  severyn: 'severyn',
  kaijin: 'kaijin',
  'al-rashid': 'al-rashid',
  'marcas-destrozadas': 'las marcas destrozadas',
};

export function findEntryForSector(sectorId, entries) {
  const key = normalize(sectorToEntryKey[sectorId] || sectorId);
  return entries.find((entry) => {
    const title = normalize(entry.title);
    const path = normalize(entry.path);
    return title.includes(key) || path.includes(key);
  }) || null;
}
