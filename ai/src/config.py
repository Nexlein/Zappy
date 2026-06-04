##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Tunable AI constants (shared across states)
##

# Food level under which survival takes priority over everything else.
SURVIVAL_THRESHOLD = 5

# Max ticks BroadcastHelp keeps calling allies before giving up.
# Safety net only: the SURVIVAL_THRESHOLD is the real abort condition.
RALLY_TIMEOUT = 100

# Ticks between two RALLY broadcasts while waiting for allies.
BCAST_INTERVAL = 10
