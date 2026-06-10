#!/bin/bash
PORT=4242
TEAM="TeamC"
SLEEP=0.3
MAP_X=15
MAP_Y=10
ITERATIONS=10

{
    echo "$TEAM"
    sleep $SLEEP

    # Scan pattern - alternate right/left turns
    for iteration in $(seq 1 $ITERATIONS); do
        for row in $(seq 1 $MAP_X); do
            # Go across row
            for col in $(seq 1 $MAP_Y); do
                echo "Take food"
                echo "Forward"
                sleep $SLEEP
            done

            # Turn around for next row
            if [ $row -lt $MAP_X ]; then
                # Odd rows: turn right, even rows: turn left
                if [ $((row % 2)) -eq 1 ]; then
                    echo "Right"
                    echo "Forward"
                    echo "Take food"
                    echo "Right"
                else
                    echo "Left"
                    echo "Forward"
                    echo "Take food"
                    echo "Left"
                fi
                sleep $SLEEP
            fi

            echo "Inventory"
        done
    done
} | nc localhost $PORT