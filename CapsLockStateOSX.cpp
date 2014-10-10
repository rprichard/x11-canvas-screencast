#include "CapsLockState.h"

#include "ObjectiveCBridge.h"

namespace screencast {

bool capsLockEnabled()
{
    return bridgeCapsLockEnabled();
}

} // namespace screencast
