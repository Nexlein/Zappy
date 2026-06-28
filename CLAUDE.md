# Zappy

Network game where teams of AI players compete on a tile-based map. First team to have 6 players reach level 8 wins.

**Main Binaries:** `zappy_server` (C++), `zappy_gui` (C++), `zappy_ai` (Python)
**Bonus Binary:** `zappy_tui` (Python orchestrator)

## The World & Elevation

- Map: flat, toroidal
- Resources spawn every 20 time units based on map size & density
- 1 food unit = 126 time units of life
- Elevation: Players gather on a tile with required stones and incant. All must be same level.

| Level | Players | linemate | deraumere | sibur | mendiane | phiras | thystame |
|-------|---------|----------|-----------|-------|----------|--------|----------|
| 1→2   | 1       | 1        | 0         | 0     | 0        | 0      | 0        |
| 2→3   | 2       | 1        | 1         | 1     | 0        | 0      | 0        |
| 3→4   | 2       | 2        | 0         | 1     | 0        | 2      | 0        |
| 4→5   | 4       | 1        | 1         | 2     | 0        | 1      | 0        |
| 5→6   | 4       | 1        | 2         | 1     | 3        | 0      | 0        |
| 6→7   | 6       | 1        | 2         | 3     | 0        | 1      | 0        |
| 7→8   | 6       | 2        | 2         | 2     | 2        | 2      | 1        |

## Commands

```bash
./zappy_server -p port -x width -y height -n name1 name2 ... -c clientsNb -f freq
./zappy_gui    -p port [-h machine]
./zappy_ai     -p port -n name [-h machine]
./zappy_tui    # (Bonus)
```

---

## Workflow

**Commits:** `type: short description` (Types: `feat`, `fix`, `chore`, `refactor`, `docs`, `test`)

**PRs:** Use `.github/pull_request_template.md` (check off tests)

**Issues:** `[Component] Short description`. Labels: `gui`, `server`, `ai`, `tui`, `feature`, `bug`, `bonus`.

## Component Docs

Keep docs updated when architecture/APIs change.
- **Server:** `server/doc.md`
- **GUI:** `gui/doc.md` & Protocol in `G-YEP-400_zappy_GUI_protocol.pdf`
- **AI:** `ai/README.md`
- **TUI (Bonus):** `tui/`
