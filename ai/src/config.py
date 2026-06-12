##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Tunable AI constants (shared across states)
##

# ── Survival ---

# Food count below which survival takes absolute priority over everything else.
SURVIVAL_THRESHOLD = 5

# Food count at which ForageFood considers the buffer "full" and go to SearchStone.
FOOD_TARGET = 15

# ForageFood turns Right or Left every N forward steps to avoid walking in a
# straight line indefinitely.
EXPLORE_TURN_EVERY = 5

# Ticks between two periodic Inventory refreshes issued by the FSM.
INVENTORY_REFRESH_INTERVAL = 15

# ── Evolution ---

# Maximum player level.
MAX_LEVEL = 8

# Incantations at this level and below can be done solo (no teammates needed).
SOLO_INCANTATION_LEVEL = 1

# ── Swarm ---

# Max ticks BroadcastHelp waits for allies before giving up and retrying solo.
# The SURVIVAL_THRESHOLD is the primary abort condition; this is a safety net.
RALLY_TIMEOUT = 100

# Ticks between successive RALLY broadcasts.
BCAST_INTERVAL = 10

# ── Network ---

# Size of the TCP receive buffer in bytes.
TCP_RECV_BUFFER_SIZE = 4096

# Seconds to sleep between non-blocking poll attempts when nothing is available.
POLL_SLEEP_SEC = 0.005
