#include "Cursor.h"

#include <QImage>
#include <QString>
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>

#include <cstdlib>
#include <set>
#include <vector>

#include <stdint.h>

#include "CaptureConfig.h"
#include "MurmurHash3.h"

namespace screencast {

std::set<uint32_t> Cursor::m_cachedImages;

Cursor::Cursor() : m_imageID(0)
{
}

Cursor::Cursor(const CaptureConfig &config)
{
    XFixesCursorImage *cursor = XFixesGetCursorImage(QX11Info::display());
    std::vector<uint32_t> pixels(cursor->width * cursor->height);
    // The X11 data is encoded as one-pixel-per-long, but each pixel is only
    // 32 bits of data, so every other 4 bytes is padding.  The Qt API is more
    // reasonable and expects one-pixel-per-32-bits, so copy the pixels.  I
    // wonder whether endianness is an issue here.
    for (size_t i = 0; i < pixels.size(); ++i)
        pixels[i] = cursor->pixels[i];
    MurmurHash3_x86_32(
                pixels.data(),
                pixels.size() * sizeof(uint32_t),
                0,
                &m_imageID);
    if (m_cachedImages.find(m_imageID) == m_cachedImages.end()) {
        QImage mouseCursor((unsigned char*)pixels.data(),
                           cursor->width, cursor->height,
                           QImage::Format_ARGB32_Premultiplied);
        mouseCursor.save(QString("cursor_%0.png").arg(m_imageID));
        m_cachedImages.insert(m_imageID);
    }
    m_position.rx() = cursor->x - cursor->xhot - config.captureX;
    m_position.ry() = cursor->y - cursor->yhot - config.captureY;
    XFree(cursor);
}

} // namespace screencast
