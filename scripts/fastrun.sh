#!/usr/bin/env bash
# fastrun.sh — hammer the "loners" setup forever, keep the fastest game.
#
# Usage: ./fastrun.sh [ais] [port_base] [workers]
#   ais        starting AIs to spawn (default 1; the queen forks up to 6+)
#   port_base  lowest port to probe (default 7000; clear of the TUI's
#              8000-8999 range and bench.sh's 6768/4242)
#   workers    parallel runs in flight (default = half your cores). Each
#              worker owns a disjoint port subrange, so they never collide.
#
# Parallelism:
#   A run is one server + a few AIs, almost all I/O-wait at freq=100 — it
#   pins well under one core. With N cores you can keep ~N/2..N runs in
#   flight and multiply throughput without touching the server's tick rate.
#   Each worker probes its OWN port subrange (no two workers can pick the
#   same free port), and best/wins/runs totals are merged under an flock.
#
# Display:
#   A live dashboard repaints in place: the session best + win/run totals on
#   top, then one line per worker (current elapsed + its last result). The
#   full per-run history is appended to fastrun_logs/results.log.
#
# Robustness contract:
#   * Never touches a process it didn't spawn. Each run is its own session
#     (setsid); teardown is `kill -- -PGID`, never pkill/fuser by name.
#   * No zombies (leader waited on), no orphan AIs (session kill is recursive),
#     no leaked ports (foreground server + group kill + fresh port each run).
#   * Ctrl-C / TERM / HUP / crash all unwind cleanly via traps. Every worker
#     traps too, so a terminal Ctrl-C tears down each in-flight run deliberately.

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
CORES=$(nproc 2>/dev/null || echo 2)
WORKERS=${3:-$(( CORES > 2 ? (CORES + 1) / 2 : 1 ))}   # default ~half the cores
WORKER_SPAN=20              # ports reserved per worker (one run at a time)
TIMEOUT=80                  # s, per run before declaring it stuck

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
[ "$WORKERS" -ge 1 ] 2>/dev/null || { echo "fastrun: workers must be >= 1" >&2; exit 1; }
mkdir -p "$RUNDIR"

WORK=$(mktemp -d)
LOCK="$WORK/lock"           # flock target for shared-state updates
STATE="$WORK/state"         # "runs wins best_ticks best_run" (best_ticks '-' = unset)
# Seed the bar-to-beat from the existing best.log so a slower session can NEVER
# clobber the all-time best — we only overwrite it when a run truly beats it.
# best_run=0 marks a seeded (prior-session) best.
seed_best='-'
if [ -f "$BEST_LOG" ]; then
    seed_best=$(sed -n 's/.*took: [0-9]* s (\([0-9]*\) ticks).*/\1/p' "$BEST_LOG" | head -1)
    [ -z "$seed_best" ] && seed_best='-'
fi
printf '0 0 %s 0\n' "$seed_best" >"$STATE"
: >"$LOCK"
# Per-worker live status files (phase elapsed last_ticks last_secs) start idle.
for ((w = 1; w <= WORKERS; w++)); do printf 'idle 0 - -\n' >"$WORK/w$w"; done

# --- colors (only when writing to a terminal) -------------------------------
if [ -t 1 ]; then
    B=$'\e[1m'; D=$'\e[2m'; R=$'\e[0m'
    GRN=$'\e[32m'; RED=$'\e[31m'; CYN=$'\e[36m'; YLW=$'\e[33m'; MAG=$'\e[35m'
    TTY=1
else
    B= D= R= GRN= RED= CYN= YLW= MAG=; TTY=0
fi

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

# Atomically publish a worker's live status (rename = no torn reads).
stat_write() {              # $1=file $2=phase $3=elapsed $4=last_ticks $5=last_secs
    printf '%s %s %s %s\n' "$2" "$3" "$4" "$5" >"$1.tmp" && mv -f "$1.tmp" "$1"
}

# --- shared-state merge (called by workers under flock) ---------------------
# Bumps the global run counter, folds in a win, and adopts a new best (copying
# its log) atomically. Sets GLOB_RUNS for the caller's results.log line.
update_state() {            # $1 = ticks ("" on fail), $2 = log path for best.log
    local ticks=$1 logpath=$2 cr cw cbt cbr
    exec 9>"$LOCK"; flock 9
    read -r cr cw cbt cbr <"$STATE"
    cr=$((cr + 1))
    if [ -n "$ticks" ]; then
        cw=$((cw + 1))
        if [ "$cbt" = "-" ] || [ "$ticks" -lt "$cbt" ]; then
            cbt=$ticks; cbr=$cr
            cp -f "$logpath" "$BEST_LOG"
        fi
    fi
    printf '%s %s %s %s\n' "$cr" "$cw" "$cbt" "$cbr" >"$STATE"
    flock -u 9; exec 9>&-
    GLOB_RUNS=$cr
}

# --- live dashboard ---------------------------------------------------------
BOARD_LINES=$((WORKERS + 1))   # 1 summary line + one per worker
PAINTED=0
render_board() {
    [ "$TTY" -eq 1 ] || return 0
    local cr cw cbt cbr i phase el lt ls last bestcol
    read -r cr cw cbt cbr <"$STATE" 2>/dev/null || return 0
    [ "$PAINTED" -eq 1 ] && printf '\e[%dA' "$BOARD_LINES"
    PAINTED=1
    local bestref="run $cbr"
    [ "$cbr" = "0" ] && bestref="prev"
    if [ "$cbt" = "-" ]; then bestcol="${D}—${R}"; bestref="-"; else bestcol="${GRN}${B}${cbt}t${R}"; fi
    printf '\e[K %sbest%s %s %s(%s)%s   %s·%s   %s%s wins%s / %s runs\n' \
        "$B" "$R" "$bestcol" "$D" "$bestref" "$R" "$D" "$R" "$GRN" "$cw" "$R" "$cr"
    for ((i = 1; i <= WORKERS; i++)); do
        read -r phase el lt ls <"$WORK/w$i" 2>/dev/null || { phase=idle; el=0; lt=-; ls=-; }
        if [ "$lt" = "-" ]; then last="${D}—${R}"; else last="${lt}t ${D}${ls}s${R}"; fi
        case $phase in
            run)   printf '\e[K %sw%02d%s  %srunning%s %3ss      last %b\n' \
                       "$D" "$i" "$R" "$CYN" "$R" "$el" "$last" ;;
            setup) printf '\e[K %sw%02d%s  %ssetup%s   %3ss      last %b\n' \
                       "$D" "$i" "$R" "$YLW" "$R" "$el" "$last" ;;
            *)     printf '\e[K %sw%02d%s  %sidle%s             last %b\n' \
                       "$D" "$i" "$R" "$D" "$R" "$last" ;;
        esac
    done
}

print_summary() {
    local cr cw cbt cbr
    read -r cr cw cbt cbr <"$STATE" 2>/dev/null || return 0
    printf '\n'
    local bestref="run $cbr"
    [ "$cbr" = "0" ] && bestref="prev session"
    if [ "$cbt" != "-" ]; then
        printf '%s  best: %s%s ticks%s (%s) %s· %s wins / %s runs%s\n' \
            "$B" "$GRN" "$cbt" "$R$B" "$bestref" "$D" "$cw" "$cr" "$R"
        printf '  best game log: %s%s%s\n' "$CYN" "$BEST_LOG" "$R"
    else
        printf '  no finished game yet (%s runs)\n' "$cr"
    fi
}

# --- helpers ----------------------------------------------------------------
# Lowest port in [base, base+span) with nothing listening (passive ss probe —
# never opens a connection, so it can't masquerade as a client to a server).
find_free_port() {          # $1 = base, $2 = span
    local base=$1 span=$2 p busy
    busy=$(ss -ltnH 2>/dev/null | awk '{print $4}' | sed 's/.*://')
    for ((p = base; p < base + span; p++)); do
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

# Watch a run, publishing live elapsed to the worker's status file every
# second. rc 0 = win, rc 1 = stuck/crash/timeout.
monitor_run() {             # $1=log $2=pgid $3=statfile $4=last_ticks $5=last_secs
    local log=$1 pgid=$2 stat=$3 lt=$4 ls=$5 start el
    start=$SECONDS
    while :; do
        grep -q "ticks) to win" "$log" 2>/dev/null && return 0
        kill -0 -- "-$pgid" 2>/dev/null || return 1   # session gone, no win
        el=$((SECONDS - start))
        [ "$el" -ge "$TIMEOUT" ] && return 1
        stat_write "$stat" run "$el" "$lt" "$ls"
        sleep 1
    done
}

# --- one worker: launch/monitor/teardown runs forever -----------------------
worker_loop() {
    local wid=$1 base=$2 RUNNING_PGID="" port snap log ticks secs
    local STAT="$WORK/w$wid" lt="-" ls="-"
    # Each worker tears down only its own in-flight run, then exits.
    trap '[ -n "$RUNNING_PGID" ] && kill_group "$RUNNING_PGID"; exit 0' INT TERM HUP

    while :; do
        stat_write "$STAT" setup 0 "$lt" "$ls"
        port=$(find_free_port "$base" "$WORKER_SPAN") || { sleep 2; continue; }

        snap="$WORK/snap.$wid"
        /bin/ls "$SRV_LOGDIR"/server_p${port}_*.log 2>/dev/null | sort >"$snap"

        # server + AIs as ONE session -> $! is the session/group leader (pgid).
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
        RUNNING_PGID=$!

        log=$(find_log "$port" "$snap")
        if [ -z "$log" ]; then
            kill_group "$RUNNING_PGID"; RUNNING_PGID=""
            continue
        fi

        if monitor_run "$log" "$RUNNING_PGID" "$STAT" "$lt" "$ls"; then
            ticks=$(sed -n 's/.*took: [0-9]* s (\([0-9]*\) ticks).*/\1/p' "$log" | head -1)
            secs=$(sed -n  's/.*took: \([0-9]*\) s ([0-9]* ticks).*/\1/p' "$log" | head -1)
        else
            ticks=""; secs=""
        fi
        kill_group "$RUNNING_PGID"; RUNNING_PGID=""

        update_state "$ticks" "$log"      # sets GLOB_RUNS
        if [ -n "$ticks" ]; then
            lt=$ticks; ls=$secs
            printf '%s  run %04d  %s ticks  %ss\n' \
                "$(date '+%F %T')" "$GLOB_RUNS" "$ticks" "$secs" >>"$RESULTS"
        else
            lt="TO"; ls="-"
            printf '%s  run %04d  TIMEOUT/FAIL\n' \
                "$(date '+%F %T')" "$GLOB_RUNS" >>"$RESULTS"
        fi
        rm -f "$log"            # keep server/logs from growing without bound
    done
}

# --- header -----------------------------------------------------------------
rule() { printf '%s%s%s\n' "$D" "────────────────────────────────────────────────────" "$R"; }
printf '%s fastrun %s %s%dx%d%s  c=%d f=%d  %s%s/%s%s  ais=%d  %s%d workers%s  port %d+\n' \
    "$MAG$B" "$R" "$B" "$WIDTH" "$HEIGHT" "$R" "$CLIENTS" "$FREQ" \
    "$YLW" "$TEAM" "$STRATEGY" "$R" "$AIS" "$CYN$B" "$WORKERS" "$R" "$PORT_BASE"
printf '%s score: ticks (lower=faster)  timeout %ds  logs: %s/   Ctrl-C to stop%s\n' \
    "$D" "$TIMEOUT" "$RUNDIR" "$R"
rule

# --- spawn workers ----------------------------------------------------------
WPIDS=()
for ((w = 1; w <= WORKERS; w++)); do
    worker_loop "$w" "$((PORT_BASE + (w - 1) * WORKER_SPAN))" &
    WPIDS+=("$!")
done

# Terminal Ctrl-C reaches the workers directly (same process group); they each
# tear their run down via their own trap. We just stop rendering and announce.
STOP=0
trap 'STOP=1' INT TERM HUP
trap 'print_summary; rm -rf "$WORK"' EXIT

# Main = the renderer. Repaint until told to stop, then reap every worker.
while [ "$STOP" -eq 0 ]; do
    render_board
    sleep 1
done
printf '%s\n%sfastrun: stopping...%s\n' "" "$YLW" "$R"
for p in "${WPIDS[@]}"; do
    while kill -0 "$p" 2>/dev/null; do wait "$p" 2>/dev/null; done
done
