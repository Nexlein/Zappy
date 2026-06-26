# Finite State Machine (FSM)

Controls drone logic via state transitions.

## States

- **ForageFood**: Gathers food to survive.
- **SearchStone**: Looks for stones to elevate.
- **Incantation**: Initiates or joins elevation ritual.
- **BroadcastHelp**: Asks swarm for missing stones.
- **MapsToAlly**: Moves to broadcasting ally.
- **Reproduce**: Forks new drones.

## Flow

1. `AIController.tick()` calls `CurrentState.update()`.
2. Evaluates transitions.
3. If no transition, `CurrentState.get_action()` sends command to server.
