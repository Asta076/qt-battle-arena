#include "audiomanager.h"

AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
{
    m_player   = new QMediaPlayer(this);
    m_audioOut = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOut);
    m_audioOut->setVolume(0.6f);
}

void AudioManager::playMusic(const QString& path, bool loop)
{
    if (path == m_currentPath) return;   // ← ADD THIS
    m_currentPath = path;
    m_player->setSource(QUrl("qrc:" + path));
    m_player->setLoops(loop ? QMediaPlayer::Infinite : 1);
    m_player->play();
}

void AudioManager::stopMusic() { m_player->stop(); }

void AudioManager::setMusicVolume(float v) { m_audioOut->setVolume(v); }

void AudioManager::playSfx(const QString& path)
{
    // QSoundEffect is fire-and-forget, perfect for short clips
    auto* sfx = new QSoundEffect(this);
    sfx->setSource(QUrl("qrc:" + path));
    sfx->setVolume(0.8f);
    sfx->play();
    // Clean up automatically when done
    connect(sfx, &QSoundEffect::playingChanged, this, [sfx]{
        if (!sfx->isPlaying()) sfx->deleteLater();
    });
}
