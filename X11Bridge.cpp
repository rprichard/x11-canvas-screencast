#include "X11Bridge.h"

#include <QX11Info>
#include <cstdlib>

#include <X11/XKBlib.h>

namespace screencast {

bool capsLockEnabled()
{
    unsigned int n = 0;
    XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &n);
    return (n & 0x01) == 1;
}

} // namespace screencast
