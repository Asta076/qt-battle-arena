// ─────────────────────────────────────────────────────────────────────────────
//  Battle Arena Relay Server
//
//  Compile standalone:
//    Linux/Mac:  g++ -std=c++17 -O2 -o server main.cpp -lboost_system -lpthread
//    Windows:    cl /std:c++17 /O2 main.cpp /link boost_system.lib ws2_32.lib
//
//  Run:  ./server [port]   (default port: 55000)
//
//  Each room holds exactly 2 players.
//  The server only relays messages — it has zero game logic.
// ─────────────────────────────────────────────────────────────────────────────

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <deque>

using boost::asio::ip::tcp;
using boost::system::error_code;

// ── Forward declarations ──────────────────────────────────────────────────────
class Session;
class Room;
class Server;

// ── Room ──────────────────────────────────────────────────────────────────────

class Room {
public:
    std::string code;
    std::vector<std::shared_ptr<Session>> players;   // max 2

    bool isFull()   const { return players.size() >= 2; }
    bool isEmpty()  const { return players.empty(); }

    int  role(std::shared_ptr<Session> s) const {
        for (int i = 0; i < (int)players.size(); ++i)
            if (players[i] == s) return i;
        return -1;
    }

    void broadcast(const std::string& msg,
                   std::shared_ptr<Session> exclude = nullptr);

    void remove(std::shared_ptr<Session> s);

    // For next_vote logic
    int  readyVotes = 0;
    bool p1ready    = false;
    bool p2ready    = false;
    int  p1char     = 0;
    int  p2char     = 0;
    int  nextVotes  = 0;
};

// ── Session ───────────────────────────────────────────────────────────────────

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Server& server)
        : m_socket(std::move(socket)), m_server(server) {}

    void start() { doRead(); }
    void send(const std::string& msg);

    std::string name;
    std::string roomCode;

private:
    void doRead();
    void doWrite();
    void handleMessage(const std::string& msg);

    tcp::socket          m_socket;
    Server&              m_server;
    std::string          m_readBuffer;
    std::deque<std::string> m_writeQueue;
};

// ── Server ────────────────────────────────────────────────────────────────────

class Server {
public:
    Server(boost::asio::io_context& io, uint16_t port)
        : m_acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "[server] Listening on port " << port << "\n";
        doAccept();
    }

    Room* getOrCreateRoom(const std::string& code) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return &m_rooms[code];
    }

    Room* getRoom(const std::string& code) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(code);
        return (it != m_rooms.end()) ? &it->second : nullptr;
    }

    void removeFromRoom(std::shared_ptr<Session> session);

    std::mutex m_mutex;

private:
    void doAccept();

    tcp::acceptor                              m_acceptor;
    std::unordered_map<std::string, Room>      m_rooms;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Minimal JSON helpers (same as client side, no external deps)
// ─────────────────────────────────────────────────────────────────────────────

static std::string jStr(const std::string& json, const std::string& key) {
    std::string s = "\"" + key + "\":\"";
    auto p = json.find(s);
    if (p == std::string::npos) return "";
    p += s.size();
    auto e = json.find('"', p);
    return (e == std::string::npos) ? "" : json.substr(p, e - p);
}

static int jInt(const std::string& json, const std::string& key) {
    std::string s = "\"" + key + "\":";
    auto p = json.find(s);
    if (p == std::string::npos) return 0;
    p += s.size();
    try { return std::stoi(json.substr(p)); } catch (...) { return 0; }
}

static bool jBool(const std::string& json, const std::string& key) {
    std::string s = "\"" + key + "\":";
    auto p = json.find(s);
    if (p == std::string::npos) return false;
    p += s.size();
    return json.substr(p, 4) == "true";
}

static std::string jType(const std::string& json) { return jStr(json, "type"); }

// ─────────────────────────────────────────────────────────────────────────────
//  Room implementation
// ─────────────────────────────────────────────────────────────────────────────

void Room::broadcast(const std::string& msg, std::shared_ptr<Session> exclude) {
    for (auto& p : players)
        if (p && p != exclude)
            p->send(msg);
}

void Room::remove(std::shared_ptr<Session> s) {
    players.erase(std::remove(players.begin(), players.end(), s), players.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Session implementation
// ─────────────────────────────────────────────────────────────────────────────

void Session::send(const std::string& msg) {
    bool writing = !m_writeQueue.empty();
    m_writeQueue.push_back(msg);
    if (!writing) doWrite();
}

void Session::doWrite() {
    if (m_writeQueue.empty()) return;
    auto self = shared_from_this();
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(m_writeQueue.front()),
        [this, self](error_code ec, std::size_t) {
            if (!ec) {
                m_writeQueue.pop_front();
                if (!m_writeQueue.empty()) doWrite();
            }
        });
}

void Session::doRead() {
    auto self = shared_from_this();
    boost::asio::async_read_until(
        m_socket,
        boost::asio::dynamic_buffer(m_readBuffer),
        '\n',
        [this, self](error_code ec, std::size_t bytes) {
            if (ec) {
                // Session disconnected
                m_server.removeFromRoom(self);
                return;
            }
            std::string msg = m_readBuffer.substr(0, bytes);
            m_readBuffer.erase(0, bytes);
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
                msg.pop_back();
            if (!msg.empty()) handleMessage(msg);
            doRead();
        });
}

void Session::handleMessage(const std::string& msg)
{
    std::string type = jType(msg);
    std::lock_guard<std::mutex> lock(m_server.m_mutex);

    // ── JOIN ─────────────────────────────────────────────────────────────────
    if (type == "join") {
        std::string code = jStr(msg, "room");
        name    = jStr(msg, "name");
        roomCode = code;

        Room& room = *m_server.getOrCreateRoom(code);

        if (room.isFull()) {
            send("{\"type\":\"error\",\"reason\":\"room full\"}\n");
            return;
        }

        int role = (int)room.players.size();   // 0 = host, 1 = guest
        room.players.push_back(shared_from_this());
        room.code = code;

        send("{\"type\":\"joined\",\"role\":" + std::to_string(role) + "}\n");
        std::cout << "[server] " << name << " joined room " << code
                  << " as " << (role == 0 ? "host" : "guest") << "\n";

        if (room.isFull()) {
            room.broadcast("{\"type\":\"room_ready\"}\n");
            std::cout << "[server] Room " << code << " is ready\n";
        }
        return;
    }

    // All other messages require the session to be in a room
    Room* room = m_server.getRoom(roomCode);
    if (!room) return;

    // ── READY (character selection) ───────────────────────────────────────────
    if (type == "ready") {
        int ch = jInt(msg, "character");
        int role = room->role(shared_from_this());
        if (role == 0) { room->p1char = ch; room->p1ready = true; }
        else           { room->p2char = ch; room->p2ready = true; }

        if (room->p1ready && room->p2ready) {
            std::string gs = "{\"type\":\"game_start\",\"p1char\":"
                           + std::to_string(room->p1char)
                           + ",\"p2char\":" + std::to_string(room->p2char) + "}\n";
            room->broadcast(gs);
            std::cout << "[server] Room " << roomCode << " game started\n";
        }
        return;
    }

    // ── INPUT (relay guest→host only) ─────────────────────────────────────────
    if (type == "input") {
        int role = room->role(shared_from_this());
        if (role == 1) {
            // Guest → relay to host only
            for (auto& p : room->players)
                if (p && room->role(p) == 0)
                    p->send(msg + "\n");
        }
        return;
    }

    // ── STATE (relay host→guest only) ─────────────────────────────────────────
    if (type == "state") {
        int role = room->role(shared_from_this());
        if (role == 0) {
            // Host → relay to guest only
            for (auto& p : room->players)
                if (p && room->role(p) == 1)
                    p->send(msg + "\n");
        }
        return;
    }

    // ── ROUND_END (relay host→guest) ─────────────────────────────────────────
    if (type == "round_end") {
        room->broadcast(msg + "\n", shared_from_this());
        return;
    }

    // ── NEXT_VOTE ────────────────────────────────────────────────────────────
    if (type == "next_vote") {
        bool vote = jBool(msg, "vote");
        int role  = room->role(shared_from_this());

        // Relay vote to the other player
        room->broadcast(msg + "\n", shared_from_this());

        if (!vote) {
            room->broadcast("{\"type\":\"next_result\",\"continue\":false}\n");
            room->nextVotes = 0;
        } else {
            room->nextVotes++;
            if (room->nextVotes >= 2) {
                room->nextVotes = 0;
                room->p1ready = false;
                room->p2ready = false;
                room->broadcast("{\"type\":\"next_result\",\"continue\":true}\n");
            }
        }
        return;
    }

    // ── PING ─────────────────────────────────────────────────────────────────
    if (type == "ping") {
        send("{\"type\":\"pong\"}\n");
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Server implementation
// ─────────────────────────────────────────────────────────────────────────────

void Server::doAccept()
{
    m_acceptor.async_accept(
        [this](error_code ec, tcp::socket socket) {
            if (!ec) {
                socket.set_option(tcp::no_delay(true));
                auto session = std::make_shared<Session>(
                    std::move(socket), *this);
                session->start();
                std::cout << "[server] New connection\n";
            }
            doAccept();
        });
}

void Server::removeFromRoom(std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_rooms.find(session->roomCode);
    if (it == m_rooms.end()) return;

    Room& room = it->second;
    room.remove(session);

    std::cout << "[server] " << session->name << " left room "
              << session->roomCode << "\n";

    if (room.isEmpty()) {
        m_rooms.erase(it);
        std::cout << "[server] Room " << session->roomCode << " deleted\n";
    } else {
        // Notify remaining player
        room.broadcast("{\"type\":\"room_closed\"}\n");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    uint16_t port = 55000;
    if (argc >= 2) {
        try { port = static_cast<uint16_t>(std::stoi(argv[1])); }
        catch (...) {}
    }

    try {
        boost::asio::io_context io;

        // Handle Ctrl+C gracefully
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&](error_code, int) {
            std::cout << "\n[server] Shutting down\n";
            io.stop();
        });

        Server server(io, port);
        io.run();

    } catch (const std::exception& e) {
        std::cerr << "[server] Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}