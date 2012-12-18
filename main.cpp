#include <QApplication>
#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QX11Info>
#include <cstdio>

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <stdint.h>
#include <time.h>

#include "CaptureConfig.h"
#include "Cursor.h"
#include "MurmurHash3.h"
#include "X11Bridge.h"

void sleepMS(int ms)
{
    timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("//STARTING IN 2 SECONDS\n");
    sleep(2);

    QApplication app(argc, argv);
    screencast::CaptureConfig config;

    config.captureX = 0;
    config.captureY = 25;
    config.captureWidth = 950;
    config.captureHeight = 500;
    QImage previousImage;
    screencast::Cursor previousCursor;
    bool previousFrozen = false;
    QPoint frozenMousePosition;

    while (true) {
        // Sleep so we poll the screen regularly.
        sleepMS(1000 / 30/*FPS*/);

        // Check for CAPS LOCK status.  If the key is pressed, "freeze" the
        // recording.
        const bool frozen = screencast::capsLockEnabled();
        if (previousFrozen && frozen)
            continue;

        if (!previousFrozen && frozen) {
            // Newly frozen.  Save mouse position;
            printf("//FROZEN\n");
            frozenMousePosition = QCursor::pos();
            previousFrozen = frozen;
            continue;
        }

        if (previousFrozen && !frozen) {
            // Newly unfrozen.  Warp to the frozen mouse position.  Pause for
            // another frame to give programs a chance to update the mouse
            // cursor image (and hover, etc).
            printf("//UNFROZEN\n");
            QCursor::setPos(frozenMousePosition);
            previousFrozen = frozen;
            QX11Info::display();
            // XXX: I don't know why, but constructing this Cursor is necessary
            // to force the setPos call above to take effect *before* pausing.
            screencast::Cursor flushYetAnotherQtCache(config);
            continue;
        }

        screencast::Cursor cursor(config);
        QDesktopWidget *desktop = QApplication::desktop();
        QPixmap screenshotPixmap = QPixmap::grabWindow(
                    desktop->winId(),
                    config.captureX, config.captureY,
                    config.captureWidth, config.captureHeight);
        QImage screenshot = screenshotPixmap.toImage();
        int delay = 100;

        if (screenshot != previousImage) {
            QString sampleName = QString("sample_%0.png").arg(QDateTime::currentMSecsSinceEpoch());
            previousImage = screenshot;
            screenshot.save(sampleName);
            printf("[%d,\"screen\",\"%s\"],\n", delay, sampleName.toStdString().c_str());
            delay = 0;
        }

        delay = std::min(delay, 30);

        if (cursor.position() != previousCursor.position()) {
            printf("[%d,\"cpos\",%d,%d],\n", delay, cursor.position().x(), cursor.position().y());
            delay = 0;
        }
        if (cursor.imageID() != previousCursor.imageID()) {
            printf("[%d,\"cimg\",\"cursor_%u.png\"],\n", delay, cursor.imageID());
            delay = 0;
        }
        previousCursor = cursor;
    }
}
