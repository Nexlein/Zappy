# Zappy

Network game where teams of AI players compete on a tile-based world map (Trantor). First team to have 6 players reach max elevation (level 8) wins.

**Binaries:** `zappy_server` (C), `zappy_gui` (C++), `zappy_ai` (free language)

## The World

- Map: flat, toroidal (wrap-around edges)
- Resources spawn every 20 time units: food, linemate, deraumere, sibur, mendiane, phiras, thystame
- Quantity formula: `map_width * map_height * density` (food=0.5, linemate=0.3, ..., thystame=0.05)
- Players occupy full tiles, multiple players/resources can share a tile
- 1 food unit = 126 time units of life; players start with 10 food (1260 time units)

## Elevation Ritual

Players gather on a tile and incant. All must be same level. Stones consumed on success.

| Level | Players | linemate | deraumere | sibur | mendiane | phiras | thystame |
|-------|---------|----------|-----------|-------|----------|--------|----------|
| 1→2   | 1       | 1        | 0         | 0     | 0        | 0      | 0        |
| 2→3   | 2       | 1        | 1         | 1     | 0        | 0      | 0        |
| 3→4   | 2       | 2        | 0         | 1     | 0        | 2      | 0        |
| 4→5   | 4       | 1        | 1         | 2     | 0        | 1      | 0        |
| 5→6   | 4       | 1        | 2         | 1     | 3        | 0      | 0        |
| 6→7   | 6       | 1        | 2         | 3     | 0        | 1      | 0        |
| 7→8   | 6       | 2        | 2         | 2     | 2        | 2      | 1        |

## Binaries & Args

```
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
./zappy_gui    -p port [-h machine]
./zappy_ai     -p port -n name [-h machine]
```

- Server: single process, single thread, uses `poll()` for socket multiplexing
- GUI authenticates with team name `GRAPHIC`
- AI is fully autonomous after launch

---

## Commits

Conventional Commits format:

```
<type>: <short description>
```

Types: `feat`, `fix`, `chore`, `refactor`, `docs`, `test`

Examples from history:
- `feat: add permanent selection on double-click`
- `fix: initialize id and tileX/Y in getEmptySelection`
- `chore: ran linter`

---

## Pull Requests

Template (`.github/pull_request_template.md`):
```
# Changes
# Related   <!-- Closes #X -->
# Tests
- [ ] Tests pass locally
```

---

## Issues

Title norm: `[Component] Short description`

Components: `[GUI]`, `[Server]`, `[AI]`

Labels:

| Label          | Use                          |
|----------------|------------------------------|
| `gui`          | GUI client work              |
| `server`       | Server work                  |
| `ai`           | AI client work               |
| `feature`      | New functionality            |
| `bug`          | Something broken             |
| `architecture` | Design / refactoring         |
| `testing`      | Tests                        |
| `documentation`| Docs                         |
| `bonus`        | Polish / extras              |
| `blocked`      | Blocked by dependency        |

---

## Documentation Policy

When a feature is fully implemented or an architectural change is made:
- Ask the user whether they want docs updated, and how
- If the change affects a component's architecture, data flow, or public API, the relevant doc file (`gui/doc.md`, etc.) likely needs updating
- If a new doc file is added or a component's scope changes significantly, reflect it in this `CLAUDE.md` under Component Docs
- Major architectural decisions (new patterns, new subsystems, significant refactors) should be documented — do not silently skip them

## Component Docs

- **GUI:** `gui/doc.md` — architecture, data flow, rendering design, file structure
- **Server:** no doc file yet — see `server/` sources directly
- **AI:** no doc file yet — see `ai/` sources directly
- **GUI protocol** (server↔GUI messages): `G-YEP-400_zappy_GUI_protocol.pdf`
- **Full subject** (game rules, commands, elevation table): `G-YEP-400_zappy.pdf`
