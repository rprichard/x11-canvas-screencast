#include "Cursor.h"

#include <QImage>
#include <QString>

#include <cstdlib>
#include <set>

#include <stdint.h>

namespace screencast {

uint32_t Cursor::blankCursor()
{
    const uint32_t blankId = 0;

    if (m_cachedImages.find(blankId) == m_cachedImages.end()) {
        uint32_t pixel = 0;
        QImage mouseCursor(
            reinterpret_cast<unsigned char*>(&pixel),
            1, 1,
            QImage::Format_ARGB32_Premultiplied);
        mouseCursor.save(QString("cursor_%0.png").arg(blankId));
        m_cachedImages.insert(blankId);
    }
    return blankId;
}

} // namespace screencast
