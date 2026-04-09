#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QUrl>

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);

    void playMusic(const QString& resourcePath, bool loop = true);
    void stopMusic();
    void playSfx(const QString& resourcePath);
    void setMusicVolume(float v); // 0.0 – 1.0

private:
    QMediaPlayer*  m_player;
    QAudioOutput*  m_audioOut;
    QString        m_currentPath;   // ← ADD THIS
};
