#!/bin/bash

# Trap Ctrl+C and script exit to kill all background processes
trap 'echo "Stopping all processes..."; kill $(jobs -p) 2>/dev/null; exit' EXIT INT TERM

echo "======================"
echo "    Zappy Launcher    "
echo "======================"
echo ""

# 1. Server config
read -p "Server binary (default: ./zappy_server): " SERVER_BIN
if [ -z "$SERVER_BIN" ]; then
    SERVER_BIN="./zappy_server"
fi
read -p "Server flags (default: -p 4242 -x 10 -y 10 -n Team1 -c 10 -f 100): " SERVER_FLAGS
if [ -z "$SERVER_FLAGS" ]; then
    SERVER_FLAGS="-p 4242 -x 10 -y 10 -n Team1 -c 10 -f 100"
fi

# 2. GUI config
read -p "GUI binary (default: ./zappy_gui): " GUI_BIN
if [ -z "$GUI_BIN" ]; then
    GUI_BIN="./zappy_gui"
fi
read -p "GUI flags (default: -p 4242 -h 127.0.0.1): " GUI_FLAGS
if [ -z "$GUI_FLAGS" ]; then
    GUI_FLAGS="-p 4242 -h 127.0.0.1"
fi

# 3. AI config
read -p "AI binary (default: ./zappy_ai): " AI_BIN
if [ -z "$AI_BIN" ]; then
    AI_BIN="./zappy_ai"
fi
read -p "How many AIs to start? (default: 0): " AI_COUNT
if [ -z "$AI_COUNT" ]; then
    AI_COUNT=0
fi

if [ "$AI_COUNT" -gt 0 ]; then
    read -p "Are all AIs using the same flags? (Y/n): " SAME_AI
    if [[ "$SAME_AI" != "n" && "$SAME_AI" != "N" ]]; then
        read -p "AI base flags (default: -p 4242 -n Team1 -h 127.0.0.1): " AI_FLAGS
        if [ -z "$AI_FLAGS" ]; then
            AI_FLAGS="-p 4242 -n Team1 -h 127.0.0.1"
        fi
        
        read -p "AI Strategy (fsm/utility, default: fsm): " AI_STRAT
        if [ -n "$AI_STRAT" ]; then
            AI_FLAGS="$AI_FLAGS -s $AI_STRAT"
        fi
        
        read -p "Verbose output? (y/N): " AI_VERB
        if [[ "$AI_VERB" == "y" || "$AI_VERB" == "Y" ]]; then
            AI_FLAGS="$AI_FLAGS -v"
        fi

        for ((i=1; i<=AI_COUNT; i++)); do
            AI_FLAGS_ARRAY[$i]=$AI_FLAGS
        done
    else
        for ((i=1; i<=AI_COUNT; i++)); do
            read -p "AI $i base flags: " AI_FLAGS
            read -p "AI $i Strategy (fsm/utility): " AI_STRAT
            if [ -n "$AI_STRAT" ]; then
                AI_FLAGS="$AI_FLAGS -s $AI_STRAT"
            fi
            read -p "AI $i Verbose? (y/N): " AI_VERB
            if [[ "$AI_VERB" == "y" || "$AI_VERB" == "Y" ]]; then
                AI_FLAGS="$AI_FLAGS -v"
            fi
            AI_FLAGS_ARRAY[$i]=$AI_FLAGS
        done
    fi
fi

echo ""
echo "=> Starting Server: $SERVER_BIN $SERVER_FLAGS"
$SERVER_BIN $SERVER_FLAGS &
SERVER_PID=$!

# Wait for server to boot
sleep 1

if [ "$AI_COUNT" -gt 0 ]; then
    echo "=> Starting $AI_COUNT AIs..."
    for ((i=1; i<=AI_COUNT; i++)); do
        $AI_BIN ${AI_FLAGS_ARRAY[$i]} &
    done
fi

echo "=> Starting GUI: $GUI_BIN $GUI_FLAGS"
$GUI_BIN $GUI_FLAGS

# When GUI exits, the trap at the top will kill the background server and AIs.
