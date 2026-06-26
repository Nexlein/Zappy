#include "RaylibAudioManager.hpp"

void RaylibAudioManager::init() { InitAudioDevice(); }

void RaylibAudioManager::shutdown()
{
    for (auto& [key, sound] : _sounds) UnloadSound(sound);
    _sounds.clear();
    CloseAudioDevice();
}

void RaylibAudioManager::playSound(const std::string& key, const std::string& filepath)
{
    if (!_sounds.contains(key)) _sounds[key] = LoadSound(filepath.c_str());
    PlaySound(_sounds[key]);
}
