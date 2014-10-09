#import "CapsLockState.h"

#import <AppKit/AppKit.h>

namespace screencast {

bool capsLockEnabled()
{
    return [NSEvent modifierFlags] & NSAlphaShiftKeyMask;
}

} // namespace screencast
