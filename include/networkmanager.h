#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>
#include <memory>
#include <string>
#include <deque>
#include <mutex>


// ─────────────────────────────────────────────────────────────────────────────
//  NetworkManager
//
//  Runs a Boost.Asio TCP client on a private QThread.
//  All Qt signals are emitted via queued connections so they arrive safely
//  on the main (Qt) thread — no manual mutex needed in UI code.
//
//  Lifecycle:
//    1. Construct on main thread.
//    2. Call connectToServer() — starts the background thread.
//    3. Use send() from any thread — it is thread-safe.
//    4. Handle incoming messages via the messageReceived() signal.
//    5. Call shutdown() before destroying (or just destroy — destructor handles it).
// ─────────────────────────────────────────────────────────────────────────────
class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;

    // Connect to relay server. Non-blocking — emits connected() or connectionFailed()
    void connectToServer(const QString& host, quint16 port);

    // Thread-safe. Queues message for sending. Returns immediately.
    void send(const std::string& message);

    // Graceful shutdown — stops IO thread, closes socket
    void shutdown();

    bool isConnected() const { return m_connected.load(); }

signals:
    void connected();
    void connectionFailed(const QString& reason);
    void disconnected(const QString& reason);
    void messageReceived(const QString& message);   // emitted on main thread

private:
    void runIoThread(const std::string& host, uint16_t port);
    void doRead();
    void doWrite();
    void emitMessage(const std::string& msg);

    // IO thread
    std::unique_ptr<QThread>                    m_thread;

    // Socket — accessed only from IO thread
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // Send queue — shared between main thread and IO thread
    std::deque<std::string>   m_sendQueue;
    std::mutex                m_sendMutex;

    std::atomic<bool>         m_connected { false };
    std::atomic<bool>         m_stopping  { false };

    // Read buffer — IO thread only
    std::string m_readBuffer;

    static constexpr uint16_t DEFAULT_PORT = 55000;
};