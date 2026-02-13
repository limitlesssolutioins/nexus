import { useMemo, useState } from 'react';
import './index.css';
import MapView from './components/MapView';
import EntryContent from './components/EntryContent';
import {
  STORY,
  buildEntryIndex,
  buildGraph,
  findEntryForSector,
  getTypeLabel,
  groupByType,
  searchEntries,
} from './lib/content';

const VIEW_OPTIONS = [
  { id: 'wiki', label: 'Wiki' },
  { id: 'timeline', label: 'Timeline' },
  { id: 'map', label: 'Mapa' },
  { id: 'graph', label: 'Grafo' },
];

const entries = STORY.entries || [];

function App() {
  const [view, setView] = useState('wiki');
  const [query, setQuery] = useState('');
  const [selectedId, setSelectedId] = useState(entries[0]?.id || null);

  const { byId } = useMemo(() => buildEntryIndex(entries), []);
  const graph = useMemo(() => buildGraph(STORY.relations || []), []);
  const filteredEntries = useMemo(() => searchEntries(entries, query), [query]);
  const grouped = useMemo(() => groupByType(filteredEntries), [filteredEntries]);
  const selectedEntry = selectedId ? byId.get(selectedId) : null;

  const handleWikiLink = (target) => {
    if (!target) return;
    const normalized = target
      .normalize('NFD')
      .replace(/[\u0300-\u036f]/g, '')
      .toLowerCase()
      .trim();
    const next = entries.find((entry) => {
      const title = entry.title
        .normalize('NFD')
        .replace(/[\u0300-\u036f]/g, '')
        .toLowerCase();
      const path = entry.path
        .normalize('NFD')
        .replace(/[\u0300-\u036f]/g, '')
        .toLowerCase();
      return title.includes(normalized) || path.includes(normalized);
    });
    if (next) setSelectedId(next.id);
  };

  const incoming = graph.incoming.get(selectedId) || [];
  const outgoing = graph.outgoing.get(selectedId) || [];

  return (
    <div className="nexus-app">
      <aside className="sidebar">
        <div className="brand">
          <p className="brand-kicker">StoryBible Interactivo</p>
          <h1>Nexus</h1>
          <p className="brand-meta">
            {STORY.summary?.totalEntries || 0} entradas · {STORY.summary?.totalRelations || 0} enlaces
          </p>
        </div>

        <div className="search-wrap">
          <input
            type="search"
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            placeholder="Buscar personajes, lore, facciones..."
            className="search-input"
          />
        </div>

        <div className="entry-list">
          {Object.entries(grouped).map(([type, typeEntries]) => (
            <section key={type} className="entry-group">
              <h2>{getTypeLabel(type)}</h2>
              {typeEntries.map((entry) => (
                <button
                  key={entry.id}
                  type="button"
                  className={`entry-item ${selectedId === entry.id ? 'active' : ''}`}
                  onClick={() => setSelectedId(entry.id)}
                >
                  <span>{entry.title}</span>
                  <small>{entry.path}</small>
                </button>
              ))}
            </section>
          ))}
        </div>
      </aside>

      <main className="workspace">
        <header className="toolbar">
          <nav className="tabs">
            {VIEW_OPTIONS.map((option) => (
              <button
                key={option.id}
                type="button"
                className={`tab ${view === option.id ? 'active' : ''}`}
                onClick={() => setView(option.id)}
              >
                {option.label}
              </button>
            ))}
          </nav>
          <p className="selected-path">{selectedEntry?.path || 'Sin seleccion'}</p>
        </header>

        <section className="panel">
          {view === 'wiki' && selectedEntry && (
            <>
              <div className="entry-header">
                <h2>{selectedEntry.title}</h2>
                <div className="chips">
                  <span className="chip">{getTypeLabel(selectedEntry.type)}</span>
                  {(selectedEntry.tags || []).map((tag) => (
                    <span key={tag} className="chip secondary">
                      #{tag}
                    </span>
                  ))}
                </div>
              </div>
              <EntryContent entry={selectedEntry} onLinkClick={handleWikiLink} />
            </>
          )}

          {view === 'timeline' && (
            <div className="timeline">
              {(STORY.timelineEvents || []).map((event, index) => (
                <article key={`${event.sourceId}-${index}`} className="timeline-event">
                  <p className="timeline-era">{event.era}</p>
                  <p>{event.label}</p>
                  <button type="button" onClick={() => setSelectedId(event.sourceId)} className="ghost-link">
                    Ver fuente
                  </button>
                </article>
              ))}
            </div>
          )}

          {view === 'map' && (
            <MapView
              onSelectSector={(sectorId) => {
                const entry = findEntryForSector(sectorId, entries);
                if (entry) setSelectedId(entry.id);
              }}
            />
          )}

          {view === 'graph' && selectedEntry && (
            <div className="graph-view">
              <article>
                <h3>Conexiones salientes</h3>
                {outgoing.length === 0 && <p className="muted">Sin conexiones registradas.</p>}
                {outgoing.map((rel, index) => {
                  const target = byId.get(rel.to);
                  if (!target) return null;
                  return (
                    <button
                      key={`out-${index}`}
                      type="button"
                      className="graph-link"
                      onClick={() => setSelectedId(target.id)}
                    >
                      {target.title}
                    </button>
                  );
                })}
              </article>
              <article>
                <h3>Conexiones entrantes</h3>
                {incoming.length === 0 && <p className="muted">Nadie referencia esta entrada.</p>}
                {incoming.map((rel, index) => {
                  const source = byId.get(rel.from);
                  if (!source) return null;
                  return (
                    <button
                      key={`in-${index}`}
                      type="button"
                      className="graph-link"
                      onClick={() => setSelectedId(source.id)}
                    >
                      {source.title}
                    </button>
                  );
                })}
              </article>
            </div>
          )}
        </section>
      </main>
    </div>
  );
}

export default App;
