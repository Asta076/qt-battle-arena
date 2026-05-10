#pragma once
#include <string>
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
//  NetworkMessage
//
//  All messages between client and server are newline-delimited JSON strings.
//  This header defines the type constants and a minimal builder/parser so
//  both client and server use identical message formats.
//
//  Intentionally pure C++ (no Qt, no Boost) so it compiles in both the
//  client and the standalone server binary without pulling in dependencies.
// ─────────────────────────────────────────────────────────────────────────────

namespace NM {

// ── Message type strings ──────────────────────────────────────────────────────
constexpr const char* JOIN         = "join";        // client → server: join room
constexpr const char* JOINED       = "joined";      // server → client: confirmed, role assigned
constexpr const char* ROOM_READY   = "room_ready";  // server → both: both players connected
constexpr const char* READY        = "ready";       // client → server: character selected, ready
constexpr const char* GAME_START   = "game_start";  // server → both: both ready, start
constexpr const char* INPUT        = "input";       // client A → server → client B: key state
constexpr const char* STATE        = "state";       // host → server → guest: authoritative state
constexpr const char* ROUND_END    = "round_end";   // server → both: someone died
constexpr const char* NEXT_VOTE    = "next_vote";   // client → server: vote to continue
constexpr const char* NEXT_RESULT  = "next_result"; // server → both: both voted / timeout
constexpr const char* ROOM_CLOSED  = "room_closed"; // server → remaining: other player left
constexpr const char* ERROR_MSG    = "error";       // server → client: something went wrong
constexpr const char* PING         = "ping";        // client → server: keepalive
constexpr const char* PONG         = "pong";        // server → client: keepalive reply

// ── Minimal JSON builder ──────────────────────────────────────────────────────
// Not a full JSON library — just enough for our fixed message formats.
// Each build_* function returns a complete message ready to send (with \n).

inline std::string build_join(const std::string& roomCode,
                               const std::string& playerName)
{
    return "{\"type\":\"join\",\"room\":\"" + roomCode
         + "\",\"name\":\"" + playerName + "\"}\n";
}

inline std::string build_joined(int role)   // role: 0=host, 1=guest
{
    return "{\"type\":\"joined\",\"role\":" + std::to_string(role) + "}\n";
}

inline std::string build_room_ready()
{
    return "{\"type\":\"room_ready\"}\n";
}

inline std::string build_ready(int characterType)
{
    return "{\"type\":\"ready\",\"character\":"
         + std::to_string(characterType) + "}\n";
}

inline std::string build_game_start(int p1char, int p2char)
{
    return "{\"type\":\"game_start\",\"p1char\":"
         + std::to_string(p1char) + ",\"p2char\":"
         + std::to_string(p2char) + "}\n";
}

// keys: bitmask — bit0=W, bit1=S, bit2=A, bit3=D, bit4=J, bit5=K
inline std::string build_input(uint32_t keyMask, uint32_t seq)
{
    return "{\"type\":\"input\",\"keys\":"
         + std::to_string(keyMask) + ",\"seq\":"
         + std::to_string(seq) + "}\n";
}

// Authoritative state sent by host every tick
inline std::string build_state(float p1x, float p1y, int p1hp, int p1sp,
                                float p2x, float p2y, int p2hp, int p2sp,
                                uint32_t seq)
{
    return "{\"type\":\"state\""
           ",\"p1x\":"  + std::to_string(p1x) +
           ",\"p1y\":"  + std::to_string(p1y) +
           ",\"p1hp\":" + std::to_string(p1hp) +
           ",\"p1sp\":" + std::to_string(p1sp) +
           ",\"p2x\":"  + std::to_string(p2x) +
           ",\"p2y\":"  + std::to_string(p2y) +
           ",\"p2hp\":" + std::to_string(p2hp) +
           ",\"p2sp\":" + std::to_string(p2sp) +
           ",\"seq\":"  + std::to_string(seq)  + "}\n";
}

inline std::string build_round_end(int winner)  // winner: 1 or 2
{
    return "{\"type\":\"round_end\",\"winner\":"
         + std::to_string(winner) + "}\n";
}

inline std::string build_next_vote(bool vote)
{
    return std::string("{\"type\":\"next_vote\",\"vote\":")
         + (vote ? "true" : "false") + "}\n";
}

inline std::string build_next_result(bool continueGame)
{
    return std::string("{\"type\":\"next_result\",\"continue\":")
         + (continueGame ? "true" : "false") + "}\n";
}

inline std::string build_room_closed()
{
    return "{\"type\":\"room_closed\"}\n";
}

inline std::string build_error(const std::string& reason)
{
    return "{\"type\":\"error\",\"reason\":\"" + reason + "\"}\n";
}

inline std::string build_ping() { return "{\"type\":\"ping\"}\n"; }
inline std::string build_pong() { return "{\"type\":\"pong\"}\n"; }

// ── Minimal JSON field extractor ──────────────────────────────────────────────
// Extracts a field value from a flat JSON string without a full parser.
// Works for our fixed message formats only — don't use for arbitrary JSON.

inline std::string extract_string(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

inline int extract_int(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    try { return std::stoi(json.substr(pos)); }
    catch (...) { return 0; }
}

inline float extract_float(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return 0.0f;
    pos += search.size();
    try { return std::stof(json.substr(pos)); }
    catch (...) { return 0.0f; }
}

inline bool extract_bool(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    return json.substr(pos, 4) == "true";
}

inline std::string extract_type(const std::string& json)
{
    return extract_string(json, "type");
}

} // namespace NM