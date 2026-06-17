#!/bin/bash
# Usage: ./test_server.sh [port] [freq] [moves]

PORT=${1:-6767}
FREQ=${2:-100}
MOVES=${3:-20}
STEP=$(awk "BEGIN { printf \"%.3f\", 8.0 / $FREQ }")

DIRS=(Forward Forward Forward Forward Right Left)

rand_cmds() {
    for _ in $(seq 1 "$MOVES"); do
        echo "${DIRS[RANDOM % ${#DIRS[@]}]}"
    done
    echo "Look"
    echo "Inventory"
}

run_client() {
    local team=$1
    (
        echo "$team"
        rand_cmds | while IFS= read -r cmd; do
            sleep "$STEP"
            echo "$cmd"
        done
        sleep 1
    ) | nc localhost "$PORT" | while IFS= read -r line; do
        printf "[%-6s] %s\n" "$team" "$line"
        if [[ "$line" == "dead" ]]; then
            printf "[%-6s] starved — killing test\n" "$team"
            kill 0
        fi
    done
}

echo "=== teama + teamb (freq=$FREQ, step=${STEP}s, moves=$MOVES) ==="

(echo "GRAPHIC"; sleep $((MOVES / 10 + 5))) | nc -q 0 localhost "$PORT" | grep "^ppo" | sed 's/^/[GUI   ] /' &

run_client "teama" &
run_client "teamb" &

wait
echo "=== done ==="
