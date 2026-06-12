#pragma once

#include "core/data/Orientation.hpp"

/// Returns the direction (1-8) a receiver hears a broadcast from, relative to their orientation.
/// Returns 0 if emitter and receiver are on the same tile.
/// Uses shortest toroidal path.
inline int broadcastDirection(int srcX, int srcY, int dstX, int dstY, int mapW, int mapH,
                              Orientation dstFacing)
{
    if (srcX == dstX && srcY == dstY) return 0;

    // shortest toroidal delta from dst to src
    int dx = srcX - dstX;
    int dy = srcY - dstY;
    if (dx > mapW / 2) dx -= mapW;
    if (dx < -mapW / 2) dx += mapW;
    if (dy > mapH / 2) dy -= mapH;
    if (dy < -mapH / 2) dy += mapH;

    // rotate delta to be relative to receiver's facing direction
    // N: no rotation, E: rotate -90, S: rotate 180, W: rotate 90
    int rx, ry;
    switch (dstFacing) {
        case Orientation::N:
            rx = dx;
            ry = -dy;
            break;
        case Orientation::E:
            rx = dy;
            ry = dx;
            break;
        case Orientation::S:
            rx = -dx;
            ry = dy;
            break;
        case Orientation::W:
            rx = -dy;
            ry = -dx;
            break;
        default:
            rx = dx;
            ry = -dy;
            break;
    }

    // map to directions 1-8: 1=front, 2=front-right, ..., 8=front-left
    if (ry > 0 && rx == 0) return 1;
    if (ry > 0 && rx > 0) return 2;
    if (ry == 0 && rx > 0) return 3;
    if (ry < 0 && rx > 0) return 4;
    if (ry < 0 && rx == 0) return 5;
    if (ry < 0 && rx < 0) return 6;
    if (ry == 0 && rx < 0) return 7;
    if (ry > 0 && rx < 0) return 8;
    return 0;
}
