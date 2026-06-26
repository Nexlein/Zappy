#!/usr/bin/env bash
# fastrun.sh — hammer the "loners" setup forever, keep the fastest game.
#
# Usage: ./fastrun.sh [ais] [port_base]
#   ais        starting AIs to spawn (default 1; the queen forks up to 6+)
#   port_base  lowest port to probe (default 7000; clear of the TUI's
#              8000-8999 range and bench.sh's 6768/4242)
#
# Robustness contract:
#   * Never touches a process it didn't spawn. Each run is its own session
#     (setsid); teardown is `kill -- -PGID`, never pkill/fuser by name.
#   * No zombies (leader waited on), no orphan AIs (session kill is recursive),
#     no leaked ports (foreground server + group kill + fresh port each run).
#   * Ctrl-C / TERM / HUP / crash all unwind cleanly via traps. The run lives
#     in its own session, so a terminal Ctrl-C reaches only THIS script, which
#     then tears the run down deliberately.

set -u

# --- config (mirrors profiles.toml [profiles.solo] "loners") ----------------
WIDTH=7
HEIGHT=7
CLIENTS=4
FREQ=100
TEAM=loners
STRATEGY=queen

AIS=${1:-1}                 # loners has ai=0 (runs nothing); default to 1
PORT_BASE=${2:-7000}
PORT_SPAN=100               # probe [PORT_BASE, PORT_BASE+PORT_SPAN)
TIMEOUT=120                 # s (2 min), per run before declaring it stuck

SERVER=./zappy_server
AI=./zappy_ai
SRV_LOGDIR=server/logs      # where zappy_server writes its own logs

RUNDIR=fastrun_logs         # our output lives here, not in the repo root
RESULTS="$RUNDIR/results.log"
BEST_LOG="$RUNDIR/best.log"
# ----------------------------------------------------------------------------

cd "$(dirname "$0")" || exit 1
command -v ss >/dev/null || { echo "fastrun: needs 'ss' (iproute2)" >&2; exit 1; }
[ -x "$SERVER" ] || { echo "fastrun: $SERVER not found/built" >&2; exit 1; }
[ -x "$AI" ]     || { echo "fastrun: $AI not found/built" >&2; exit 1; }
mkdir -p "$RUNDIR"

WORK=$(mktemp -d)
RUNNING_PGID=""             # session of the in-flight run, for the traps
best_ticks=""              # lowest ticks seen this session
best_run=0
runs=0
wins=0

# --- colors (only when writing to a terminal) -------------------------------
if [ -t 1 ]; then
    B=$'\e[1m'; D=$'\e[2m'; R=$'\e[0m'
    GRN=$'\e[32m'; RED=$'\e[31m'; CYN=$'\e[36m'; YLW=$'\e[33m'; MAG=$'\e[35m'
else
    B= D= R= GRN= RED= CYN= YLW= MAG=
fi
CLR=$'\r\e[K'              # carriage return + clear to end of line

# --- teardown ---------------------------------------------------------------
# Kill exactly one run's session: server + every AI + their children.
# TERM first, escalate to KILL, then reap the leader so no zombie lingers.
kill_group() {
    local pgid=$1 i
    [ -z "$pgid" ] && return 0
    kill -TERM -- "-$pgid" 2>/dev/null
    for i in $(seq 1 20); do
        kill -0 -- "-$pgid" 2>/dev/null || break
        sleep 0.1
    done
    kill -KILL -- "-$pgid" 2>/dev/null
    wait "$pgid" 2>/dev/null    # leader is our direct child -> reaped here
}

print_summary() {
    printf '%s\n' "$R"
    if [ -n "$best_ticks" ]; then
        printf '%s  best this session: %s%s ticks%s (run %s) %s· %s wins / %s runs%s\n' \
            "$B" "$GRN" "$best_ticks" "$R$B" "$best_run" "$D" "$wins" "$runs" "$R"
        printf '  best game log: %s%s%s\n' "$CYN" "$BEST_LOG" "$R"
    else
        printf '  no finished game yet (%s runs)\n' "$runs"
    fi
}

cleanup() {
    trap - INT TERM HUP EXIT    # no re-entry while we tear down
    [ -n "$RUNNING_PGID" ] && kill_group "$RUNNING_PGID"
    print_summary
    rm -rf "$WORK"
}
trap cleanup EXIT
trap 'printf "%s\n%sfastrun: stopping...%s\n" "$CLR" "$YLW" "$R"; exit 130' INT
trap 'exit 143' TERM HUP

# --- helpers ----------------------------------------------------------------
# Lowest port in our range with nothing listening (passive ss probe — never
# opens a connection, so it can't masquerade as a client to a live server).
find_free_port() {
    local p busy
    busy=$(ss -ltnH 2>/dev/null | awk '{print $4}' | sed 's/.*://')
    for ((p = PORT_BASE; p < PORT_BASE + PORT_SPAN; p++)); do
        grep -qx "$p" <<<"$busy" || { echo "$p"; return 0; }
    done
    return 1
}

# Print our run's freshly-created server log path (or fail after ~10s).
find_log() {
    local port=$1 snap=$2 tries new
    for tries in $(seq 1 40); do
        new=$(comm -13 "$snap" \
              <(/bin/ls "$SRV_LOGDIR"/server_p${port}_*.log 2>/dev/null | sort) \
              | head -1)
        [ -n "$new" ] && { echo "$new"; return 0; }
        sleep 0.25
    done
    return 1
}

# Watch a run, drawing a live status line. rc 0 = win, rc 1 = stuck/crash/TO.
monitor_run() {
    local log=$1 pgid=$2 tag=$3 start now el
    start=$SECONDS
    while :; do
        grep -q "ticks) to win" "$log" 2>/dev/null && return 0
        kill -0 -- "-$pgid" 2>/dev/null || return 1   # session gone, no win
        el=$((SECONDS - start))
        [ "$el" -ge "$TIMEOUT" ] && return 1
        printf '%s%s  %srunning%s  %ss%s' "$CLR" "$tag" "$CYN" "$R$D" "$el" "$R"
        sleep 1
    done
}

# --- header -----------------------------------------------------------------
rule() { printf '%s%s%s\n' "$D" "────────────────────────────────────────────────────" "$R"; }
printf '%s fastrun %s %s%dx%d%s  c=%d f=%d  %s%s/%s%s  ais=%d  port %d+\n' \
    "$MAG$B" "$R" "$B" "$WIDTH" "$HEIGHT" "$R" "$CLIENTS" "$FREQ" \
    "$YLW" "$TEAM" "$STRATEGY" "$R" "$AIS" "$PORT_BASE"
printf '%s score: ticks (lower=faster)   logs: %s/   Ctrl-C to stop%s\n' \
    "$D" "$RUNDIR" "$R"
rule

# --- main loop --------------------------------------------------------------
while :; do
    runs=$((runs + 1))
    stamp=$(date '+%F %T')
    tag=$(printf '%srun %04d%s' "$B" "$runs" "$R")

    port=$(find_free_port) || {
        printf '%s%s  %sno free port%s\n' "$CLR" "$tag" "$RED" "$R"; sleep 2; continue; }

    snap="$WORK/snap"
    /bin/ls "$SRV_LOGDIR"/server_p${port}_*.log 2>/dev/null | sort >"$snap"

    # Launch server + AIs as ONE session. Direct setsid (no command-subst, so
    # nothing holds a pipe open) -> $! is the session/group leader (pgid).
    setsid bash -c '
        port=$1; ais=$2; srv=$3; ai=$4
        w=$5; h=$6; team=$7; c=$8; f=$9; strat=${10}
        "$srv" -p "$port" -x "$w" -y "$h" -n "$team" -c "$c" -f "$f" \
            >/dev/null 2>&1 &
        server_pid=$!
        for ((i = 0; i < ais; i++)); do
            "$ai" -p "$port" -n "$team" -s "$strat" >/dev/null 2>&1 &
        done
        wait "$server_pid"
    ' _ "$port" "$AIS" "$SERVER" "$AI" \
        "$WIDTH" "$HEIGHT" "$TEAM" "$CLIENTS" "$FREQ" "$STRATEGY" \
        >/dev/null 2>&1 </dev/null &
    pgid=$!
    RUNNING_PGID=$pgid

    log=$(find_log "$port" "$snap")
    if [ -z "$log" ]; then
        kill_group "$pgid"; RUNNING_PGID=""
        printf '%s%s  %sserver never logged (port race?)%s\n' "$CLR" "$tag" "$RED" "$R"
        continue
    fi

    if monitor_run "$log" "$pgid" "$tag"; then
        ticks=$(sed -n 's/.*took: [0-9]* s (\([0-9]*\) ticks).*/\1/p' "$log" | head -1)
        secs=$(sed -n  's/.*took: \([0-9]*\) s ([0-9]* ticks).*/\1/p' "$log" | head -1)
    else
        ticks=""
    fi

    kill_group "$pgid"; RUNNING_PGID=""

    if [ -n "$ticks" ]; then
        wins=$((wins + 1))
        if [ -z "$best_ticks" ] || [ "$ticks" -lt "$best_ticks" ]; then
            best_ticks=$ticks; best_run=$runs
            cp -f "$log" "$BEST_LOG"
            printf '%s%s  %s%5s ticks%s  %ss   %s★ NEW BEST%s\n' \
                "$CLR" "$tag" "$GRN$B" "$ticks" "$R" "$secs" "$GRN$B" "$R"
        else
            printf '%s%s  %s%5s ticks%s  %ss   %s(best %s)%s\n' \
                "$CLR" "$tag" "$B" "$ticks" "$R" "$secs" "$D" "$best_ticks" "$R"
        fi
        printf '%s  run %04d  %s ticks  %ss\n' "$stamp" "$runs" "$ticks" "$secs" >>"$RESULTS"
    else
        printf '%s%s  %sTIMEOUT/FAIL%s\n' "$CLR" "$tag" "$RED" "$R"
        printf '%s  run %04d  TIMEOUT/FAIL\n' "$stamp" "$runs" >>"$RESULTS"
    fi

    rm -f "$log"            # keep server/logs from growing without bound
done
