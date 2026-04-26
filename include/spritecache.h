#pragma once
#include <QPixmap>
#include <QMap>
#include <QString>

class SpriteCache {
public:
    static SpriteCache& instance();

    QPixmap get(const QString& resourcePath);
    QPixmap getScaled(const QString& resourcePath, int w, int h);
    
    void clear();  // call on game shutdown

private:
    SpriteCache() = default;
    QMap<QString, QPixmap> m_cache;
};
