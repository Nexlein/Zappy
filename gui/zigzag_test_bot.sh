#!/bin/bash
PORT=4242
TEAM="TeamC"
SLEEP=0.3

{
    echo "$TEAM"
    sleep $SLEEP

    # Scan pattern - alternate right/left turns
    for row in {1..10}; do
        # Go across row
        for col in {1..10}; do
            echo "Take food"
            echo "Forward"
            sleep $SLEEP
        done

        # Turn around for next row
        if [ $row -lt 10 ]; then
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
} | nc localhost $PORT