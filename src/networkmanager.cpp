#include <boost/asio.hpp>
#include "networkmanager.h"
#include <QMetaObject>

using boost::asio::ip::tcp;

// ── Private implementation ────────────────────────────────────────────────────
struct NetworkManager::Impl {
    boost::asio::io_context        io;
    tcp::socket                    socket { io };
    boost::asio::steady_timer      pingTimer { io };

    Impl() = default;
    ~Impl() = default;
};

// ─────────────────────────────────────────────────────────────────────────────

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_impl(std::make_unique<Impl>())
{}

NetworkManager::~NetworkManager()
{
    shutdown();
}

void NetworkManager::connectToServer(const QString& host, quint16 port)
{
    if (m_thread && m_thread->isRunning()) return;   // already connecting

    m_stopping  = false;
    m_connected = false;

    std::string h = host.toStdString();
    uint16_t    p = static_cast<uint16_t>(port);

    m_thread = std::make_unique<QThread>();

    // Run the entire Asio IO loop on the background thread
    QObject::connect(m_thread.get(), &QThread::started, this, [this, h, p]{
        runIoThread(h, p);
    }, Qt::DirectConnection);

    m_thread->start();
}

void NetworkManager::runIoThread(const std::string& host, uint16_t port)
{
    try {
        tcp::resolver resolver(m_impl->io);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        boost::system::error_code ec;
        boost::asio::connect(m_impl->socket, endpoints, ec);

        if (ec || m_stopping) {
            QMetaObject::invokeMethod(this, [this, ec]{
                emit connectionFailed(QString::fromStdString(ec.message()));
            }, Qt::QueuedConnection);
            return;
        }

        // Disable Nagle algorithm — critical for low latency
        m_impl->socket.set_option(tcp::no_delay(true));
        m_connected = true;

        QMetaObject::invokeMethod(this, [this]{
            emit connected();
        }, Qt::QueuedConnection);

        // Start ping timer — sends a ping every 5 seconds to keep connection alive
        // and detect dropped connections quickly
        m_impl->pingTimer.expires_after(std::chrono::seconds(5));
        m_impl->pingTimer.async_wait([this](const boost::system::error_code& e){
            if (!e && !m_stopping) {
                send("{\"type\":\"ping\"}\n");
            }
        });

        // Start async read loop
        doRead();

        // Run until shutdown() is called
        m_impl->io.run();

    } catch (const std::exception& ex) {
        QMetaObject::invokeMethod(this, [this, msg = std::string(ex.what())]{
            emit connectionFailed(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    }
}

void NetworkManager::doRead()
{
    if (m_stopping) return;

    // Read until newline — each message ends with \n
    boost::asio::async_read_until(
        m_impl->socket,
        boost::asio::dynamic_buffer(m_readBuffer),
        '\n',
        [this](const boost::system::error_code& ec, std::size_t bytes)
        {
            if (ec || m_stopping) {
                if (!m_stopping) {
                    m_connected = false;
                    QMetaObject::invokeMethod(this, [this, ec]{
                        emit disconnected(QString::fromStdString(ec.message()));
                    }, Qt::QueuedConnection);
                }
                return;
            }

            // Extract exactly one message (up to and including the \n)
            std::string msg = m_readBuffer.substr(0, bytes);
            m_readBuffer.erase(0, bytes);

            // Remove trailing whitespace
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
                msg.pop_back();

            if (!msg.empty()) {
                QMetaObject::invokeMethod(this, [this, msg]{
                    emit messageReceived(QString::fromStdString(msg));
                }, Qt::QueuedConnection);
            }

            // Read the next message
            doRead();
        }
    );
}

void NetworkManager::send(const std::string& message)
{
    if (!m_connected || m_stopping) return;

    // Push to queue (thread-safe)
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        m_sendQueue.push_back(message);
    }

    // Schedule a write on the IO thread
    boost::asio::post(m_impl->io, [this]{
        doWrite();
    });
}

void NetworkManager::doWrite()
{
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (m_sendQueue.empty()) return;
        msg = m_sendQueue.front();
        m_sendQueue.pop_front();
    }

    boost::system::error_code ec;
    boost::asio::write(m_impl->socket,
                       boost::asio::buffer(msg), ec);

    if (ec && !m_stopping) {
        m_connected = false;
        QMetaObject::invokeMethod(this, [this, ec]{
            emit disconnected(QString::fromStdString(ec.message()));
        }, Qt::QueuedConnection);
        return;
    }

    // Write next message if queue has more
    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        if (!m_sendQueue.empty()) {
            boost::asio::post(m_impl->io, [this]{ doWrite(); });
        }
    }
}

void NetworkManager::shutdown()
{
    if (m_stopping) return;
    m_stopping  = true;
    m_connected = false;

    boost::system::error_code ec;
    m_impl->pingTimer.cancel();
    m_impl->socket.shutdown(tcp::socket::shutdown_both, ec);
    m_impl->socket.close(ec);
    m_impl->io.stop();

    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(3000);
    }
}