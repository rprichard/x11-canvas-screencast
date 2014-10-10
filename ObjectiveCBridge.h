#ifndef SCREENCAST_OBJECTIVECBRIDGE_H
#define SCREENCAST_OBJECTIVECBRIDGE_H

#include <objc/runtime.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	double x;
	double y;
} BridgePoint;

id cursor_currentSystemCursor();
BridgePoint cursor_hotSpot(id cursor);
BridgePoint event_mouseLocation();
id cursor_image(id cursor);
id image_TIFFRepresentation(id image);
const void *data_bytes(id data);
size_t data_length(id data);
void bridgeWriteCursorFile(id tiffData, uint32_t imageID);
bool bridgeCapsLockEnabled();

#ifdef __cplusplus
}
#endif

#endif // SCREENCAST_OBJECTIVECBRIDGE_H
