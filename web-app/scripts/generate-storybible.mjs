import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const projectRoot = path.resolve(__dirname, '..', '..');
const storyRoot = path.join(projectRoot, 'StoryBible');
const outFile = path.join(__dirname, '..', 'src', 'content', 'generated', 'storybible.json');

function walkMarkdownFiles(dir) {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  const files = [];
  for (const entry of entries) {
    if (entry.name.startsWith('.')) continue;
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      files.push(...walkMarkdownFiles(full));
      continue;
    }
    if (entry.isFile() && entry.name.toLowerCase().endsWith('.md')) files.push(full);
  }
  return files;
}

function slugify(value) {
  return value
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .replace(/['"]/g, '')
    .replace(/[^a-zA-Z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '')
    .toLowerCase();
}

function normalize(value) {
  return value
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .replace(/\s+/g, ' ')
    .trim()
    .toLowerCase();
}

function parseFrontmatter(rawText) {
  if (!rawText.startsWith('---\n')) return { frontmatter: {}, content: rawText };
  const end = rawText.indexOf('\n---\n', 4);
  if (end === -1) return { frontmatter: {}, content: rawText };

  const fmRaw = rawText.slice(4, end);
  const content = rawText.slice(end + 5);
  const frontmatter = {};

  for (const line of fmRaw.split('\n')) {
    const clean = line.trim();
    if (!clean || clean.startsWith('#')) continue;
    const divider = clean.indexOf(':');
    if (divider === -1) continue;
    const key = clean.slice(0, divider).trim();
    let value = clean.slice(divider + 1).trim();
    if (value.startsWith('"') && value.endsWith('"')) value = value.slice(1, -1);
    if (value.startsWith("'") && value.endsWith("'")) value = value.slice(1, -1);
    if (value.startsWith('[') && value.endsWith(']')) {
      value = value
        .slice(1, -1)
        .split(',')
        .map((v) => v.trim().replace(/^['"]|['"]$/g, ''))
        .filter(Boolean);
    }
    frontmatter[key] = value;
  }

  return { frontmatter, content };
}

function detectType(relativePath) {
  const top = relativePath.split('/')[0];
  const map = {
    Chapters: 'chapter',
    Characters: 'character',
    Factions: 'faction',
    Enemies: 'enemy',
    Lore: 'lore',
    World: 'world',
    Templates: 'template',
  };
  return map[top] || 'reference';
}

function findTitle(content, fallback) {
  const heading = content
    .split('\n')
    .map((line) => line.trim())
    .find((line) => line.startsWith('# '));
  if (heading) return heading.replace(/^#\s+/, '').trim();
  return fallback;
}

function extractWikiLinks(content) {
  const links = [];
  const pattern = /\[\[([^\]]+)\]\]/g;
  let match = pattern.exec(content);
  while (match) {
    const full = match[1].trim();
    const [targetPart, labelPart] = full.split('|');
    const target = (targetPart || '').split('#')[0].trim();
    if (target) {
      links.push({
        raw: full,
        target,
        label: labelPart?.trim() || target,
      });
    }
    match = pattern.exec(content);
  }
  return links;
}

function buildLookup(entry) {
  const candidates = new Set();
  const fileName = path.basename(entry.path, '.md');
  candidates.add(normalize(entry.title));
  candidates.add(normalize(fileName));
  candidates.add(normalize(fileName.replace(/_/g, ' ')));
  if (entry.frontmatter?.titulo) candidates.add(normalize(String(entry.frontmatter.titulo)));
  return [...candidates];
}

function buildTimelineEvents(entries) {
  const timeline = entries.find((entry) => entry.path.endsWith('World/Timeline.md'));
  if (!timeline) return [];

  const events = [];
  let era = 'General';
  const lines = timeline.content.split('\n');

  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed) continue;

    const section = /^##\s+(.+)$/.exec(trimmed);
    if (section) {
      era = section[1].trim();
      continue;
    }

    const bullet = /^-\s+(.+)$/.exec(trimmed);
    if (bullet) {
      events.push({
        era,
        label: bullet[1].trim(),
        sourceId: timeline.id,
      });
      continue;
    }

    if (trimmed.startsWith('|') && !trimmed.includes('---')) {
      const cells = trimmed
        .split('|')
        .map((cell) => cell.trim())
        .filter(Boolean);
      if (cells.length >= 2 && cells[0].toLowerCase() !== 'mes' && cells[0].toLowerCase() !== 'periodo') {
        events.push({
          era,
          label: `${cells[0]}: ${cells[1]}`,
          sourceId: timeline.id,
        });
      }
    }
  }

  return events;
}

if (!fs.existsSync(storyRoot)) {
  throw new Error(`StoryBible not found at: ${storyRoot}`);
}

const files = walkMarkdownFiles(storyRoot);
const entries = files.map((filePath) => {
  const relativePath = path.relative(storyRoot, filePath).split(path.sep).join('/');
  const raw = fs.readFileSync(filePath, 'utf8');
  const { frontmatter, content } = parseFrontmatter(raw);
  const fallbackTitle = path.basename(filePath, '.md').replace(/_/g, ' ');
  const title = findTitle(content, fallbackTitle);
  const tags = Array.isArray(frontmatter.tags)
    ? frontmatter.tags
    : typeof frontmatter.tags === 'string'
      ? [frontmatter.tags]
      : [];
  const links = extractWikiLinks(content);

  return {
    id: slugify(relativePath.replace(/\.md$/i, '')),
    title,
    type: detectType(relativePath),
    path: relativePath,
    tags,
    frontmatter,
    content: content.trim(),
    links,
  };
});

const lookup = new Map();
for (const entry of entries) {
  for (const key of buildLookup(entry)) {
    if (!lookup.has(key)) lookup.set(key, entry.id);
  }
}

for (const entry of entries) {
  entry.links = entry.links.map((link) => {
    const resolvedId = lookup.get(normalize(link.target)) || null;
    return { ...link, resolvedId };
  });
}

const timelineEvents = buildTimelineEvents(entries);
const relations = [];
for (const entry of entries) {
  for (const link of entry.links) {
    if (!link.resolvedId) continue;
    relations.push({
      from: entry.id,
      to: link.resolvedId,
      label: link.label,
    });
  }
}

const payload = {
  generatedAt: new Date().toISOString(),
  projectName: 'Nexus',
  summary: {
    totalEntries: entries.length,
    totalRelations: relations.length,
    totalTimelineEvents: timelineEvents.length,
  },
  entries,
  relations,
  timelineEvents,
};

fs.mkdirSync(path.dirname(outFile), { recursive: true });
fs.writeFileSync(outFile, JSON.stringify(payload, null, 2), 'utf8');
console.log(`Generated ${entries.length} entries -> ${path.relative(projectRoot, outFile)}`);
