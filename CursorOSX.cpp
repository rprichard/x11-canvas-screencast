#include "Cursor.h"

#include <QApplication>
#include <QDesktopWidget>
#include <cassert>
#include <set>

#include <stdint.h>

#include "CaptureConfig.h"
#include "MurmurHash3.h"
#include "ObjectiveCBridge.h"

namespace screencast {

std::set<uint32_t> Cursor::m_cachedImages;

Cursor::Cursor() : m_imageID(0)
{
}

Cursor::Cursor(const CaptureConfig &config)
{
    id cursor = cursor_currentSystemCursor();
    BridgePoint hotSpot = cursor_hotSpot(cursor);
    BridgePoint mouseLocation = event_mouseLocation();
    id cursorImage = cursor_image(cursor);
    id tiffData = cursorImage != nil ? 
        image_TIFFRepresentation(cursorImage) : nil;

    if (tiffData == nil) {
        m_imageID = Cursor::blankCursor();
        m_position.rx() = 0;
        m_position.ry() = 0;
        return;
    }

    MurmurHash3_x86_32(
                data_bytes(tiffData),
                data_length(tiffData),
                0,
                &m_imageID);

    if (m_cachedImages.find(m_imageID) == m_cachedImages.end()) {
        bridgeWriteCursorFile(tiffData, m_imageID);
    }

    // The NSCursor Y coordinate's origin is at the bottom of the screen.
    // Invert it.
    mouseLocation.y = 
        QApplication::desktop()->screenGeometry().height() - mouseLocation.y;

    m_position.rx() = mouseLocation.x - hotSpot.x - config.captureX;
    m_position.ry() = mouseLocation.y - hotSpot.y - config.captureY;
}

} // namespace screencast
