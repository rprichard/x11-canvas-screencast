#ifndef SCREENCAST_CAPTURECONFIG_H
#define SCREENCAST_CAPTURECONFIG_H

namespace screencast {

struct CaptureConfig
{
    CaptureConfig() :
        captureX(0), captureY(0),
        captureWidth(0), captureHeight(0)
    {
    }

    int captureX;
    int captureY;
    int captureWidth;
    int captureHeight;
};

} // namespace screencast

#endif // SCREENCAST_CAPTURECONFIG_H
