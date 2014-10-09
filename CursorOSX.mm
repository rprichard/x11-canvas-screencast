#import "Cursor.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#import <QApplication>
#import <QDesktopWidget>
#import <cassert>
#import <set>

#import <stdint.h>

#import "CaptureConfig.h"
#import "MurmurHash3.h"

namespace screencast {

std::set<uint32_t> Cursor::m_cachedImages;

Cursor::Cursor() : m_imageID(0)
{
}

Cursor::Cursor(const CaptureConfig &config)
{
    NSCursor *cursor = [NSCursor currentSystemCursor];
    NSPoint hotSpot = [cursor hotSpot];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSData *tiffData = [[cursor image] TIFFRepresentation];

    if (tiffData == nil) {
        m_imageID = Cursor::blankCursor();
        m_position.rx() = 0;
        m_position.ry() = 0;
        return;
    }

    MurmurHash3_x86_32(
                [tiffData bytes],
                [tiffData length],
                0,
                &m_imageID);

    if (m_cachedImages.find(m_imageID) == m_cachedImages.end()) {
        NSBitmapImageRep *imageRep = 
            [[[NSBitmapImageRep alloc] initWithData:tiffData] autorelease];
        assert(imageRep != nil);
        NSData *pngData = 
            [imageRep representationUsingType:NSPNGFileType properties:nil];
        NSString *path = 
            [NSString stringWithFormat:@"cursor_%u.png", m_imageID];
        [pngData writeToFile:path atomically:NO];
    }

    // The NSCursor Y coordinate's origin is at the bottom of the screen.
    // Invert it.
    mouseLocation.y = 
        QApplication::desktop()->screenGeometry().height() - mouseLocation.y;

    m_position.rx() = mouseLocation.x - hotSpot.x - config.captureX;
    m_position.ry() = mouseLocation.y - hotSpot.y - config.captureY;
}

} // namespace screencast
