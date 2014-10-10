#include <QApplication>
#include <QDateTime>
#include <QDesktopWidget>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPoint>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "CapsLockState.h"
#include "CaptureConfig.h"
#include "Cursor.h"
#include "MurmurHash3.h"

volatile bool g_finish = false;

static void sleepMS(int ms)
{
    timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
}

static void usage(const char *argv0)
{
    printf("Usage: %s --rect X Y W H --output OUTFILE\n", argv0);
}

static bool stringEndsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.size() > str.size())
        return false;
    return !str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    screencast::CaptureConfig config;
    std::string output;
    std::string baseName;

    for (int i = 1; i < argc; ) {
        const char *arg = argv[i];
        if (!strcmp(arg, "--rect") && (i + 4 < argc)) {
            config.captureX = atoi(argv[i + 1]);
            config.captureY = atoi(argv[i + 2]);
            config.captureWidth = atoi(argv[i + 3]);
            config.captureHeight = atoi(argv[i + 4]);
            i += 5;
        } else if (!strcmp(arg, "--help")) {
            usage(argv[0]);
            exit(0);
        } else if (!strcmp(arg, "--output") && (i + 1 < argc)) {
            output = argv[i + 1];
            if (!stringEndsWith(output, ".js")) {
                fprintf(stderr, "Output file must end with .js extension.\n");
                exit(1);
            }
            baseName = output.substr(0, output.size() - 3);
            i += 2;
        } else {
            fprintf(stderr, "error: Unrecognized argument: %s\n", argv[i]);
            usage(argv[0]);
            exit(1);
        }
    }
    if (config.captureWidth == 0 || config.captureHeight == 0) {
        fprintf(stderr,
            "error: A capture rectangle must be specified with --rect.\n");
        usage(argv[0]);
        exit(1);
    }
    if (output.empty()) {
        fprintf(stderr,
            "error: An output file must be specified with --output.\n");
        usage(argv[0]);
        exit(1);
    }

    FILE *fp = fopen(output.c_str(), "wb");
    if (!fp) {
        fprintf(stderr, "error: Could not open %s\n", output.c_str());
        exit(1);
    }
    setvbuf(fp, NULL, _IOLBF, 0);
    fprintf(fp, "%s = {\n", baseName.c_str());
    fprintf(fp, "\"width\": %d,\n", config.captureWidth);
    fprintf(fp, "\"height\": %d,\n", config.captureHeight);
    fprintf(fp, "\"steps\": [\n");

    printf("Starting in 2 seconds.\n");
    sleep(2);
    printf("Now recording -- press ENTER to stop.\n");

    QImage previousImage;
    screencast::Cursor previousCursor;
    bool previousFrozen = false;
    QPoint frozenMousePosition;

    {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(STDIN_FILENO, F_SETFL, flags);
    }

    const int FPS = 60;
    const int maxDelayMS = 250;
    bool firstFrame = true;
    int delay = 0;
    char buf;
    while (read(STDIN_FILENO, &buf, 1) == -1 && errno == EAGAIN) {
        // Sleep so we poll the screen regularly.
        sleepMS(1000 / FPS);
        if (!firstFrame)
            delay = std::min(maxDelayMS, delay + 1000 / FPS);

        // Check for CAPS LOCK status.  If the key is pressed, "freeze" the
        // recording.
        const bool frozen = screencast::capsLockEnabled();
        if (previousFrozen && frozen)
            continue;

        if (!previousFrozen && frozen) {
            // Newly frozen.  Save mouse position;
            printf("FROZEN\n");
            fprintf(fp, "//FROZEN\n");
            frozenMousePosition = QCursor::pos();
            previousFrozen = frozen;
            continue;
        }

        if (previousFrozen && !frozen) {
            // Newly unfrozen.  Warp to the frozen mouse position.  Pause for
            // another frame to give programs a chance to update the mouse
            // cursor image (and hover, etc).
            printf("UNFROZEN\n");
            fprintf(fp, "//UNFROZEN\n");
            QCursor::setPos(frozenMousePosition);
            previousFrozen = frozen;
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

        if (screenshot != previousImage) {
            QString sampleName = QString("sample_%0.png").arg(QDateTime::currentMSecsSinceEpoch());
            previousImage = screenshot;
            screenshot.save(sampleName);
            fprintf(fp, "[%d,\"screen\",\"%s\"],\n", delay, sampleName.toStdString().c_str());
            delay = 0;
            firstFrame = false;
        }

        if (cursor.imageID() != previousCursor.imageID()) {
            fprintf(fp, "[%d,\"cursor\",\"cursor_%u.png\",%d,%d],\n",
                delay, cursor.imageID(),
                cursor.position().x(), cursor.position().y());
            delay = 0;
        } else if (cursor.position() != previousCursor.position()) {
            fprintf(fp, "[%d,\"cpos\",%d,%d],\n",
                delay,
                cursor.position().x(), cursor.position().y());
            delay = 0;
        }
        previousCursor = cursor;
    }

    fprintf(fp, "]};\n");
    fclose(fp);

    return 0;
}
