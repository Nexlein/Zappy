/// @file CameraController.hpp
/// @brief Owns all camera logic: orbital, freecam, follow, and smooth transitions.

#pragma once

#include "core/WorldState.hpp"
#include "raylib.h"

class CameraController {
    public:
    // ── Tuning ─────────────────────────────────────────────────────────────
    float orbitalMoveSpeed = 2.0f;
    float freecamMoveSpeed = 5.0f;
    float freecamLookSpeed = 0.002f;
    float followDist = 2.5f;
    float followHeight = 1.8f;
    float followLookOffsetY = 0.5f;
    float transitionDuration = 0.8f;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void init(float worldWidth, float worldHeight);
    void update(float dt, float worldWidth, float worldHeight, const WorldState* world);
    void handleInput();

    const Camera3D& camera() const { return _camera; }

    // ── Mode switching ─────────────────────────────────────────────────────
    void enterFreecam();
    void exitFreecam();
    void startFollow(int playerId);
    void stopFollow();

    // ── State queries ──────────────────────────────────────────────────────
    bool isFreecamActive() const { return _mode == Mode::Freecam; }
    bool isFollowActive() const { return _followedPlayerId != -1; }
    bool isTransitioning() const { return _transitionT < transitionDuration; }
    int followedPlayerId() const { return _followedPlayerId; }

    private:
    enum class Mode { Orbital, Freecam, Follow, FollowLive };

    Camera3D _camera = {};
    Mode _mode = Mode::Orbital;

    float _angle = 0.0f;
    float _height = 5.0f;
    float _savedOrbitalAngle = 0.0f;
    float _savedOrbitalHeight = 5.0f;

    float _freecamYaw = 0.0f;
    float _freecamPitch = 0.0f;

    int _followedPlayerId = -1;
    float _lastWorldWidth = 10.0f;
    float _lastWorldHeight = 10.0f;

    float _transitionT = 1.0f;
    Vector3 _fromPos = {};
    Vector3 _fromTarget = {};
    Vector3 _toPos = {};
    Vector3 _toTarget = {};
    Mode _pendingMode = Mode::Orbital;

    void _startTransition(Vector3 toPos, Vector3 toTarget, Mode nextMode);
    void _applyOrbital(float worldWidth, float worldHeight);
    void _orbitalDest(float worldWidth, float worldHeight, Vector3& outPos,
                      Vector3& outTarget) const;
    bool _followTarget(const WorldState* world, Vector3& outPos, Vector3& outLookAt) const;
    void _handleOrbitalInput();
    void _handleFreecamInput();
};
