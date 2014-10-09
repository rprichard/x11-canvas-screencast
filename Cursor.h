#ifndef SCREENCAST_CURSOR_H
#define SCREENCAST_CURSOR_H

#include <QPoint>
#include <set>

#include <stdint.h>

namespace screencast {

class CaptureConfig;

class Cursor
{
public:
    Cursor();
    Cursor(const CaptureConfig &config);
    QPoint position() { return m_position; }
    uint32_t imageID() { return m_imageID; }

private:
	static uint32_t blankCursor();

private:
    QPoint m_position;
    uint32_t m_imageID;
    static std::set<uint32_t> m_cachedImages;
};

} // namespace screencast

#endif // SCREENCAST_CURSOR_H
