function parseInline(text, onLinkClick, keyPrefix = 'part') {
  const parts = [];
  const pattern = /\[\[([^\]]+)\]\]/g;
  let last = 0;
  let idx = 0;
  let match = pattern.exec(text);

  while (match) {
    if (match.index > last) {
      parts.push(<span key={`${keyPrefix}-text-${idx}`}>{text.slice(last, match.index)}</span>);
    }
    const [targetRaw, labelRaw] = match[1].split('|');
    const target = (targetRaw || '').trim();
    const label = (labelRaw || target).trim();
    parts.push(
      <button
        key={`${keyPrefix}-link-${idx}`}
        type="button"
        className="inline-link"
        onClick={() => onLinkClick?.(target)}
      >
        {label}
      </button>,
    );
    last = match.index + match[0].length;
    idx += 1;
    match = pattern.exec(text);
  }

  if (last < text.length) {
    parts.push(<span key={`${keyPrefix}-tail`}>{text.slice(last)}</span>);
  }
  return parts;
}

export default function EntryContent({ entry, onLinkClick }) {
  if (!entry) return null;

  const lines = entry.content.split('\n');
  const nodes = [];
  let listBuffer = [];

  const flushList = () => {
    if (!listBuffer.length) return;
    nodes.push(
      <ul key={`list-${nodes.length}`}>
        {listBuffer.map((item, index) => (
          <li key={`list-item-${index}`}>{parseInline(item, onLinkClick, `li-${nodes.length}-${index}`)}</li>
        ))}
      </ul>,
    );
    listBuffer = [];
  };

  lines.forEach((rawLine, index) => {
    const line = rawLine.trimEnd();
    const trimmed = line.trim();

    if (!trimmed) {
      flushList();
      return;
    }

    if (trimmed.startsWith('- ')) {
      listBuffer.push(trimmed.slice(2));
      return;
    }

    flushList();

    if (trimmed.startsWith('### ')) {
      nodes.push(<h3 key={`h3-${index}`}>{parseInline(trimmed.slice(4), onLinkClick, `h3-${index}`)}</h3>);
      return;
    }
    if (trimmed.startsWith('## ')) {
      nodes.push(<h2 key={`h2-${index}`}>{parseInline(trimmed.slice(3), onLinkClick, `h2-${index}`)}</h2>);
      return;
    }
    if (trimmed.startsWith('# ')) {
      nodes.push(<h1 key={`h1-${index}`}>{parseInline(trimmed.slice(2), onLinkClick, `h1-${index}`)}</h1>);
      return;
    }
    if (trimmed.startsWith('>')) {
      nodes.push(
        <blockquote key={`q-${index}`}>
          {parseInline(trimmed.replace(/^>\s?/, ''), onLinkClick, `q-${index}`)}
        </blockquote>,
      );
      return;
    }
    if (trimmed.startsWith('|')) {
      nodes.push(
        <pre key={`tbl-${index}`} className="table-line">
          {trimmed}
        </pre>,
      );
      return;
    }

    nodes.push(<p key={`p-${index}`}>{parseInline(line, onLinkClick, `p-${index}`)}</p>);
  });

  flushList();
  return <div className="entry-content">{nodes}</div>;
}
