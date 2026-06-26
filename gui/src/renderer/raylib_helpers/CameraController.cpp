/// @file CameraController.cpp

#include "CameraController.hpp"

#include <cmath>

#include "raymath.h"

static float smoothstep(float t)
{
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

void CameraController::init(float worldWidth, float worldHeight)
{
    _camera = {.position = {0.0f, 10.0f, 10.0f},
               .target = {0.0f, 0.0f, 0.0f},
               .up = {0.0f, 1.0f, 0.0f},
               .fovy = 45.0f,
               .projection = CAMERA_PERSPECTIVE};
    _transitionT = transitionDuration;
    _applyOrbital(worldWidth, worldHeight);
}

void CameraController::_orbitalDest(float worldWidth, float worldHeight, Vector3& outPos,
                                    Vector3& outTarget) const
{
    float maxDim = fmaxf(worldWidth, worldHeight);
    float radius = maxDim * 1.25f;
    float aspectX = 0.5f * (1.0f + worldWidth / maxDim);
    float aspectZ = 0.5f * (1.0f + worldHeight / maxDim);
    outPos.x = cosf(_angle) * radius * aspectX;
    outPos.z = sinf(_angle) * radius * aspectZ;
    float currentRadius = sqrtf(outPos.x * outPos.x + outPos.z * outPos.z);
    outPos.y = _height * (currentRadius / radius);
    outTarget = {0.0f, 0.0f, 0.0f};
}

void CameraController::_applyOrbital(float worldWidth, float worldHeight)
{
    float maxDim = fmaxf(worldWidth, worldHeight);
    float radius = maxDim * 1.25f;
    float aspectX = 0.5f * (1.0f + worldWidth / maxDim);
    float aspectZ = 0.5f * (1.0f + worldHeight / maxDim);
    float currentRadius;

    _camera.position.x = cosf(_angle) * radius * aspectX;
    _camera.position.z = sinf(_angle) * radius * aspectZ;
    currentRadius =
        sqrtf(_camera.position.x * _camera.position.x + _camera.position.z * _camera.position.z);
    _camera.position.y = _height * (currentRadius / radius);
    _camera.target = {0.0f, 0.0f, 0.0f};
}

bool CameraController::_followTarget(const WorldState* world, Vector3& outPos,
                                     Vector3& outLookAt) const
{
    if (!world || !world->playerExists(_followedPlayerId)) return false;
    const Player& p = world->players.at(_followedPlayerId);
    Vector3 pivot = p.visual.pos;
    float yawRad = p.visual.angle * DEG2RAD;
    float facingX = -cosf(yawRad);
    float facingZ = sinf(yawRad);
    outPos = {pivot.x - facingX * followDist, pivot.y + followHeight,
              pivot.z - facingZ * followDist};
    outLookAt = {pivot.x, pivot.y + followLookOffsetY, pivot.z};
    return true;
}

void CameraController::_startTransition(Vector3 toPos, Vector3 toTarget, Mode nextMode)
{
    _fromPos = _camera.position;
    _fromTarget = _camera.target;
    _toPos = toPos;
    _toTarget = toTarget;
    _pendingMode = nextMode;
    _transitionT = 0.0f;
}

void CameraController::update(float dt, float worldWidth, float worldHeight,
                              const WorldState* world)
{
    _lastWorldWidth = worldWidth;
    _lastWorldHeight = worldHeight;
    if (_transitionT < transitionDuration) {
        _transitionT += dt;
        float alpha = smoothstep(_transitionT / transitionDuration);

        // For follow transitions: keep _toPos/_toTarget tracking the player live
        if (_pendingMode == Mode::FollowLive || _pendingMode == Mode::Follow) {
            Vector3 livePos, liveLookAt;
            if (_followTarget(world, livePos, liveLookAt)) {
                _toPos = livePos;
                _toTarget = liveLookAt;
            }
        }

        _camera.position = Vector3Lerp(_fromPos, _toPos, alpha);
        _camera.target = Vector3Lerp(_fromTarget, _toTarget, alpha);

        if (_transitionT >= transitionDuration) {
            _camera.position = _toPos;
            _camera.target = _toTarget;
            _mode = _pendingMode;
            if (_mode == Mode::Orbital) _followedPlayerId = -1;
        }
        return;
    }

    switch (_mode) {
        case Mode::Freecam: {
            Vector3 dir = {cosf(_freecamYaw) * cosf(_freecamPitch), sinf(_freecamPitch),
                           sinf(_freecamYaw) * cosf(_freecamPitch)};
            _camera.target = Vector3Add(_camera.position, dir);
            break;
        }

        case Mode::Follow:
        case Mode::FollowLive: {
            Vector3 pos, lookAt;
            if (_followTarget(world, pos, lookAt)) {
                _camera.position = pos;
                _camera.target = lookAt;
                _mode = Mode::FollowLive;
            } else {
                stopFollow();
            }
            break;
        }

        case Mode::Orbital:
            _applyOrbital(worldWidth, worldHeight);
            break;
    }
}

void CameraController::handleInput()
{
    if (isTransitioning()) return;

    if (_mode == Mode::Freecam)
        _handleFreecamInput();
    else if (_mode == Mode::Orbital)
        _handleOrbitalInput();
}

void CameraController::_handleOrbitalInput()
{
    float dt = GetFrameTime();
    // KEY_A maps to 'Q' on AZERTY
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) _angle += orbitalMoveSpeed * dt;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) _angle -= orbitalMoveSpeed * dt;
}

void CameraController::_handleFreecamInput()
{
    Vector2 delta = GetMouseDelta();
    _freecamYaw += delta.x * freecamLookSpeed;
    _freecamPitch -= delta.y * freecamLookSpeed;
    _freecamPitch = Clamp(_freecamPitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);

    Vector3 forward = {cosf(_freecamYaw) * cosf(_freecamPitch), sinf(_freecamPitch),
                       sinf(_freecamYaw) * cosf(_freecamPitch)};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f}));
    float sp = freecamMoveSpeed * GetFrameTime();

    // ZQSD on AZERTY = KEY_W/KEY_A/KEY_S/KEY_D in raylib
    if (IsKeyDown(KEY_W))
        _camera.position = Vector3Add(_camera.position, Vector3Scale(forward, sp));
    if (IsKeyDown(KEY_S))
        _camera.position = Vector3Add(_camera.position, Vector3Scale(forward, -sp));
    if (IsKeyDown(KEY_D)) _camera.position = Vector3Add(_camera.position, Vector3Scale(right, sp));
    if (IsKeyDown(KEY_A)) _camera.position = Vector3Add(_camera.position, Vector3Scale(right, -sp));
    if (IsKeyDown(KEY_SPACE)) _camera.position.y += sp;
    if (IsKeyDown(KEY_LEFT_SHIFT)) _camera.position.y -= sp;
}

void CameraController::enterFreecam()
{
    if (isTransitioning() || _mode == Mode::Freecam) return;
    _savedOrbitalAngle = _angle;
    _savedOrbitalHeight = _height;
    Vector3 dir = Vector3Subtract(_camera.target, _camera.position);
    _freecamYaw = atan2f(dir.z, dir.x);
    _freecamPitch = atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z));
    DisableCursor();
    // Freecam starts from current pos — no fly-in, just activate
    _mode = Mode::Freecam;
    _transitionT = transitionDuration;  // mark as not transitioning
}

void CameraController::exitFreecam()
{
    if (_mode != Mode::Freecam) return;
    _angle = _savedOrbitalAngle;
    _height = _savedOrbitalHeight;
    EnableCursor();
    Vector3 orbPos, orbTarget;
    _orbitalDest(_lastWorldWidth, _lastWorldHeight, orbPos, orbTarget);
    _startTransition(orbPos, orbTarget, Mode::Orbital);
}

void CameraController::startFollow(int playerId)
{
    if (isTransitioning()) return;
    if (_mode == Mode::Freecam) EnableCursor();
    _followedPlayerId = playerId;
    // Destination computed in update() live; use current camera as start
    _startTransition(_camera.position, _camera.target, Mode::Follow);
}

void CameraController::stopFollow()
{
    if (!isFollowActive()) return;
    Vector3 orbPos, orbTarget;
    _orbitalDest(_lastWorldWidth, _lastWorldHeight, orbPos, orbTarget);
    _startTransition(orbPos, orbTarget, Mode::Orbital);
}
