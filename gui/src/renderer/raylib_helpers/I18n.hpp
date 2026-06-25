#pragma once

/**
 * @brief Compile-time UI string table for EN/FR localization.
 *
 * Strings are stored as a static constexpr 2D array indexed by [language][key].
 * This gives O(1) lookup with zero allocation and no I/O — deliberately chosen over
 * a config file, since languages are a compile-time concern and a missing file is an
 * unnecessary failure mode.
 *
 * To add a language: extend the Language enum and add a column to _strings.
 * To add a string: add a Key entry and a corresponding value in each language column.
 */
class I18n {
    public:
    enum class Language { EN, FR };

    enum class Key {
        // HUD
        HUD_FPS,
        HUD_MAP,
        HUD_TIME_UNIT,
        HUD_UPTIME_UNKNOWN,
        HUD_UPTIME_PREFIX,
        HUD_UPTIME_H,
        HUD_UPTIME_M,
        HUD_UPTIME_S,
        HUD_PORT,
        HUD_MACHINE,

        // Tooltip — entity labels
        LABEL_TILE,
        LABEL_PLAYER,
        LABEL_EGG,

        // Tooltip — tile
        TILE_EMPTY,
        TILE_HEADER,

        // Tooltip — player
        PLAYER_TEAM,
        PLAYER_LEVEL,
        PLAYER_INVENTORY_EMPTY,
        PLAYER_INVENTORY,

        // Tooltip — egg
        EGG_TEAM,

        // Player label (above head)
        PLAYER_HEAD_LEVEL,

        // Win screen
        WIN_GAME_OVER,
        WIN_TEAM_WON,
        WIN_DURATION,
        WIN_DURATION_UNKNOWN,
        WIN_TICKS,
        WIN_QUIT,

        // Resource names (must stay contiguous and ordered: food=0 … thystame=6)
        RESOURCE_FOOD,
        RESOURCE_LINEMATE,
        RESOURCE_DERAUMERE,
        RESOURCE_SIBUR,
        RESOURCE_MENDIANE,
        RESOURCE_PHIRAS,
        RESOURCE_THYSTAME,

        _COUNT
    };

    static void setLanguage(Language lang) { _lang = lang; }
    static Language getLanguage() { return _lang; }

    static const char* get(Key key)
    {
        return _strings[static_cast<int>(_lang)][static_cast<int>(key)];
    }

    /** @brief Resource name by index (0–6), matching Resources[] ordering. */
    static const char* resourceName(int index)
    {
        return get(static_cast<Key>(static_cast<int>(Key::RESOURCE_FOOD) + index));
    }

    private:
    static Language _lang;

    static constexpr const char* _strings[2][static_cast<int>(Key::_COUNT)] = {
        // EN
        {
            "FPS: ",        // HUD_FPS
            "Map: ",        // HUD_MAP
            "Time unit: ",  // HUD_TIME_UNIT
            "Time --:--",   // HUD_UPTIME_UNKNOWN
            "Time: ",       // HUD_UPTIME_PREFIX
            "h ",           // HUD_UPTIME_H
            "m ",           // HUD_UPTIME_M
            "s",            // HUD_UPTIME_S
            "Port: ",       // HUD_PORT
            "Machine: ",    // HUD_MACHINE

            "Tile",    // LABEL_TILE
            "Player",  // LABEL_PLAYER
            "Egg",     // LABEL_EGG

            " is empty",  // TILE_EMPTY
            "Tile:",      // TILE_HEADER

            "  Team ",               // PLAYER_TEAM
            "  Level ",              // PLAYER_LEVEL
            "  Inventory is empty",  // PLAYER_INVENTORY_EMPTY
            "  Inventory:",          // PLAYER_INVENTORY

            "  Team ",  // EGG_TEAM

            "Level ",  // PLAYER_HEAD_LEVEL

            "Game Over",      // WIN_GAME_OVER
            " won!",          // WIN_TEAM_WON
            "Duration: ",     // WIN_DURATION
            "Duration: ...",  // WIN_DURATION_UNKNOWN
            " ticks",         // WIN_TICKS
            "Quit",           // WIN_QUIT

            "Food",       // RESOURCE_FOOD
            "Linemate",   // RESOURCE_LINEMATE
            "Deraumere",  // RESOURCE_DERAUMERE
            "Sibur",      // RESOURCE_SIBUR
            "Mendiane",   // RESOURCE_MENDIANE
            "Phiras",     // RESOURCE_PHIRAS
            "Thystame",   // RESOURCE_THYSTAME
        },
        // FR
        {
            "IPS : ",             // HUD_FPS
            "Carte : ",           // HUD_MAP
            "Unité de temps : ",  // HUD_TIME_UNIT
            "Temps --:--",        // HUD_UPTIME_UNKNOWN
            "Temps : ",           // HUD_UPTIME_PREFIX
            "h ",                 // HUD_UPTIME_H
            "m ",                 // HUD_UPTIME_M
            "s",                  // HUD_UPTIME_S
            "Port : ",            // HUD_PORT
            "Machine : ",         // HUD_MACHINE

            "Case",    // LABEL_TILE
            "Joueur",  // LABEL_PLAYER
            "Oeuf",    // LABEL_EGG

            " est vide",  // TILE_EMPTY
            "Case :",     // TILE_HEADER

            "  Équipe ",          // PLAYER_TEAM
            "  Niveau ",          // PLAYER_LEVEL
            "  Inventaire vide",  // PLAYER_INVENTORY_EMPTY
            "  Inventaire :",     // PLAYER_INVENTORY

            "  Équipe ",  // EGG_TEAM

            "Niveau ",  // PLAYER_HEAD_LEVEL

            "Fin de partie",  // WIN_GAME_OVER
            " a gagné !",     // WIN_TEAM_WON
            "Durée : ",       // WIN_DURATION
            "Durée : ...",    // WIN_DURATION_UNKNOWN
            " ticks",         // WIN_TICKS
            "Quitter",        // WIN_QUIT

            "Nourriture",  // RESOURCE_FOOD
            "Linemate",    // RESOURCE_LINEMATE
            "Deraumere",   // RESOURCE_DERAUMERE
            "Sibur",       // RESOURCE_SIBUR
            "Mendiane",    // RESOURCE_MENDIANE
            "Phiras",      // RESOURCE_PHIRAS
            "Thystame",    // RESOURCE_THYSTAME
        },
    };
};
