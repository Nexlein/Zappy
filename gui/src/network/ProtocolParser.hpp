#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "core/Event.hpp"

/**
 * @brief Parses server messages into Event objects.
 * Static class that provides a method to parse raw input strings
 * from the server and convert them into structured Event variants.
 *
 * Design pattern: FACTORY
 */
class ProtocolParser {
    public:
    /**
     * @brief Parses a raw input string from the server and converts it into an Event variant.
     * This method stops at the first '\n' character.
     * It does not provide mass transformation of multiple lines.
     * @param input The raw input string to parse.
     * @return An optional Event variant containing the parsed event if successful, or std::null
     */
    static std::optional<Event> parse(std::string_view input);

    private:
    static std::vector<std::string_view> _split(std::string_view str, char delimiter,
                                                char stopAt = '\n');
    static std::string _joinTokens(const std::vector<std::string_view>& tokens, size_t start);

    static std::optional<Event> _parseWelcome(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseMSZ(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseBCT(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseTNA(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePNW(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePPO(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePLV(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePIN(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePEX(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePBC(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePIC(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePIE(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePFK(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePDR(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePGT(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parsePDI(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseENW(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseEBO(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseEDI(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSGT(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSST(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSEG(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSMG(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSUC(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSBP(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSTU(const std::vector<std::string_view>& tokens);
    static std::optional<Event> _parseSSE(const std::vector<std::string_view>& tokens);
};
