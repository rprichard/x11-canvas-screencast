// qmake does not compile Objective-C++ sources with C++ flags.  This is a
// critical failing, because the C++ flags can alter the library ABI.  In
// particular, -stdlib=libc++ enables an incompatible standard library.
//
// I do not see an obvious non-hacky fix for this issue, and without one,
// Objective-C++ is unusable with qmake, at least on OS X with the version of
// Qt I'm using (Qt 4.8.6 installed via Homebrew).
//
// As a workaround, put Objective-C code here and call it from C++.

#import "ObjectiveCBridge.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

id cursor_currentSystemCursor()
{
    return [NSCursor currentSystemCursor];
}

BridgePoint cursor_hotSpot(id cursor)
{
    NSPoint ret = [(NSCursor*)cursor hotSpot];
    BridgePoint ret2 = { ret.x, ret.y };
    return ret2;
}

BridgePoint event_mouseLocation()
{
    NSPoint ret = [NSEvent mouseLocation];
    BridgePoint ret2 = { ret.x, ret.y };
    return ret2;
}

id cursor_image(id cursor)
{
    return [(NSCursor*)cursor image];
}

id image_TIFFRepresentation(id image)
{
    return [(NSImage*)image TIFFRepresentation];
}

const void *data_bytes(id data)
{
    return [(NSData*)data bytes];
}

size_t data_length(id data)
{
    return [(NSData*)data length];
}

void bridgeWriteCursorFile(id tiffData, uint32_t imageID)
{
    NSBitmapImageRep *imageRep =
        [[[NSBitmapImageRep alloc]
            initWithData:(NSData*)tiffData] autorelease];
    assert(imageRep != nil);
    NSData *pngData =
        [imageRep representationUsingType:NSPNGFileType properties:nil];
    NSString *path =
        [NSString stringWithFormat:@"cursor_%u.png", imageID];
    [pngData writeToFile:path atomically:NO];
}

bool bridgeCapsLockEnabled()
{
    return ([NSEvent modifierFlags] & NSAlphaShiftKeyMask) != 0;
}
