#!/bin/bash
PORT=4242

# TeamA - player 1: zigzag forward/right
connect_teamA1() {
    {
        echo "TeamA"
        sleep 0.4
        for _ in $(seq 1 20); do
            echo "Forward"; sleep 0.3
            echo "Forward"; sleep 0.3
            echo "Right";   sleep 0.3
            echo "Forward"; sleep 0.3
            echo "Forward"; sleep 0.3
            echo "Left";    sleep 0.3
        done
    } | nc localhost $PORT
}

# TeamA - player 2: spin and move
connect_teamA2() {
    {
        echo "TeamA"
        sleep 0.6
        for _ in $(seq 1 20); do
            echo "Forward"; sleep 0.25
            echo "Forward"; sleep 0.25
            echo "Forward"; sleep 0.25
            echo "Right";   sleep 0.25
        done
    } | nc localhost $PORT
}

# TeamB - player 1: wide left sweeps
connect_teamB1() {
    {
        echo "TeamB"
        sleep 0.4
        for _ in $(seq 1 15); do
            echo "Forward"; sleep 0.35
            echo "Forward"; sleep 0.35
            echo "Forward"; sleep 0.35
            echo "Left";    sleep 0.35
            echo "Forward"; sleep 0.35
            echo "Left";    sleep 0.35
        done
    } | nc localhost $PORT
}

# TeamB - player 2: slow wander
connect_teamB2() {
    {
        echo "TeamB"
        sleep 0.8
        for _ in $(seq 1 25); do
            echo "Forward"; sleep 0.5
            echo "Right";   sleep 0.5
            echo "Forward"; sleep 0.5
            echo "Forward"; sleep 0.5
            echo "Left";    sleep 0.5
            echo "Forward"; sleep 0.5
        done
    } | nc localhost $PORT
}

# TeamC - player 1: tight circles
connect_teamC1() {
    {
        echo "TeamC"
        sleep 0.4
        for _ in $(seq 1 30); do
            echo "Forward"; sleep 0.2
            echo "Right";   sleep 0.2
            echo "Forward"; sleep 0.2
            echo "Right";   sleep 0.2
        done
    } | nc localhost $PORT
}

# TeamC - player 2: long straights with u-turns
connect_teamC2() {
    {
        echo "TeamC"
        sleep 1.0
        for _ in $(seq 1 10); do
            for _ in $(seq 1 5); do
                echo "Forward"; sleep 0.3
            done
            echo "Right";   sleep 0.3
            echo "Right";   sleep 0.3
        done
    } | nc localhost $PORT
}

connect_teamA1 &
connect_teamA2 &
connect_teamB1 &
connect_teamB2 &
connect_teamC1 &
connect_teamC2 &

wait
