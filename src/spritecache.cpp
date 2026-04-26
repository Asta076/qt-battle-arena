#include "spritecache.h"
#include <QDebug>

SpriteCache& SpriteCache::instance()
{
    static SpriteCache s_instance;
    return s_instance;
}

QPixmap SpriteCache::get(const QString& resourcePath)
{
    if (m_cache.contains(resourcePath))
        return m_cache[resourcePath];

    QPixmap px(resourcePath);
    if (px.isNull())
        qWarning() << "Failed to load:" << resourcePath;
    
    m_cache[resourcePath] = px;
    return px;
}

QPixmap SpriteCache::getScaled(const QString& resourcePath, int w, int h)
{
    QString key = QString("%1_%2x%3").arg(resourcePath).arg(w).arg(h);
    
    if (m_cache.contains(key))
        return m_cache[key];

    QPixmap original = get(resourcePath);
    QPixmap scaled = original.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
    m_cache[key] = scaled;
    return scaled;
}

void SpriteCache::clear()
{
    m_cache.clear();
}
