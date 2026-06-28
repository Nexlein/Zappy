#!/usr/bin/env bash
# Usage: ./bench.sh [runs]

set -u

# config
RUNS=${1:-5}
WIDTH=10
HEIGHT=10
CLIENTS=3
FREQ=100
TEAMS=(TeamA TeamB TeamC)
AIS_PER_TEAM=1
TIMEOUT=600            # s

OUR_BIN=./zappy_server
REF_BIN=./bin/zappy_server
AI_BIN=./zappy_ai

OUR_PORT=6768
REF_PORT=4242

# --------------------------

TMP=$(mktemp -d)
PIDS=()
AI_PAT="ai/src/main.py"

kill_ais() { pkill -f "$AI_PAT" 2>/dev/null; }
free_port() { fuser -k "$1/tcp" 2>/dev/null; }   # kill whatever still holds the port

cleanup() {
    for p in "${PIDS[@]:-}"; do kill "$p" 2>/dev/null; done
    kill_ais
    free_port "$OUR_PORT"
    free_port "$REF_PORT"
    rm -rf "$TMP"
}
# EXIT runs cleanup; INT/TERM exit (which triggers EXIT) instead of leaving orphans.
trap cleanup EXIT
trap 'exit 130' INT TERM

# $1=binary  $2=port  $3=out file
start_server() {
    local bin=$1 port=$2 out=$3
    free_port "$port"   # clear any stale server before binding
    stdbuf -oL "$bin" -p "$port" -x "$WIDTH" -y "$HEIGHT" -n "${TEAMS[@]}" \
        -c "$CLIENTS" -f "$FREQ" >"$out" 2>&1 &
    local srv=$!
    PIDS+=("$srv")      # so cleanup kills it even on Ctrl-C mid-run
    sleep 1
    # for team in "${TEAMS[@]}"; do                                         -> uncomment to test multiple teams
        for _ in $(seq 1 "$AIS_PER_TEAM"); do
            # "$AI_BIN" -p "$port" -n "$team" -s queen >/dev/null 2>&1 &      -> uncomment to test multiple teams
            "$AI_BIN" -p "$port" -n "TeamA" -s queen >/dev/null 2>&1 &      # -> comment to test multiple teams
        done
    # done                                                                  -> uncomment to test multiple teams
    echo "$srv"
}

# Block until the game-over line appears in $1 (or TIMEOUT).
wait_done() {
    local out=$1 pat=$2 waited=0
    while ! grep -q "$pat" "$out"; do
        sleep 1
        waited=$((waited + 1))
        if [ "$waited" -ge "$TIMEOUT" ]; then break; fi
    done
}

# ours: "[..] [INFO] [GAME] Duration: 175 s 338500 us" (log prefix varies, so
# locate the value relative to the "Duration:" keyword)
parse_ours() {
    awk '{ for (i = 1; i <= NF; i++) if ($i == "Duration:") {
               print $(i + 1) + $(i + 3) / 1000000; exit } }' "$1"
}
# ref: "The game ended in 191 seconds ans 73112 microseconds"
parse_ref() {
    awk '/game ended in/ { print $5 + $8 / 1000000; exit }' "$1"
}

our_times=()
ref_times=()

printf "%-5s | %-14s | %-14s\n" "run" "ours (s)" "ref (s)"
printf -- "------+----------------+----------------\n"

for i in $(seq 1 "$RUNS"); do
    # both servers run in parallel so either GUI can be attached during the run
    our_srv=$(start_server "$OUR_BIN" "$OUR_PORT" "$TMP/our_$i.txt")
    ref_srv=$(start_server "$REF_BIN" "$REF_PORT" "$TMP/ref_$i.txt")

    wait_done "$TMP/our_$i.txt" "GAME OVER"
    wait_done "$TMP/ref_$i.txt" "game ended in"

    ot=$(parse_ours "$TMP/our_$i.txt")
    rt=$(parse_ref "$TMP/ref_$i.txt")

    kill "$our_srv" "$ref_srv" 2>/dev/null
    kill_ais
    free_port "$OUR_PORT"
    free_port "$REF_PORT"
    sleep 0.3

    our_times+=("${ot:-NA}")
    ref_times+=("${rt:-NA}")
    printf "%-5s | %-14s | %-14s\n" "$i" "${ot:-TIMEOUT}" "${rt:-TIMEOUT}"
done

mean() {
    printf '%s\n' "$@" | awk '
        /^[0-9.]+$/ { sum += $1; n++ }
        END { if (n) printf "%.3f", sum / n; else printf "NA" }'
}

printf -- "------+----------------+----------------\n"
printf "%-5s | %-14s | %-14s\n" "mean" "$(mean "${our_times[@]}")" "$(mean "${ref_times[@]}")"
