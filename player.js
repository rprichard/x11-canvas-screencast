function Player(script, scriptDir)
{
    var that = this;

    var kMaxCursorWidth = 32;
    var kMaxCursorHeight = 32;

    var g_scriptDir = null;
    var g_script = null;
    var g_index = -1;
    var g_divElement = null;
    var g_mainCanvas = null;
    var g_cursorCanvas = null;

    var g_blitImage = null;

    var g_imageCache = {};
    var g_imageCount = 0;
    var g_imageLoadCount = 0;

    var g_loaded = false;
    var g_paused = true;
    var g_stepTimeoutID = null;

    this.onload = function() {};
    this.onloop = function() {};

    function assert(x) {
        // With IE9, window.console is undefined (unless the console is
        // opened), so we must avoid calling console.assert.
        if (!x)
            throw new Error("Assertion failed");
    }

    function imageLoaded()
    {
        g_imageLoadCount++;
        if (g_imageLoadCount == g_imageCount)
            finishSetup();
    }

    function finishSetup()
    {
        // Avoid flicker by executing the initial 0-delay steps before firing
        // the onload handler.  The canvas will be painted when the handler
        // fires.
        assert(!g_loaded);
        for (g_index = 0; 1; g_index++) {
            assert(g_index < g_script.length);
            var delayMS = g_script[g_index][0];
            if (delayMS > 0)
                break;
            executeStep(g_index);
        }

        that.onload();

        // The onload handler could have called start or pause, so we must wait
        // until now to set g_loaded, and we must wait to read g_paused until
        // now.
        g_loaded = true;
        if (!g_paused)
            beginCurrentStep();
    }

    function executeStep(index)
    {
        var stepKind = g_script[index][1];
        if (stepKind == "blitimg") {
            var url = g_scriptDir + "/" + g_script[index][2];
            g_blitImage = g_imageCache[url];
        } else if (stepKind == "blit") {
            var ctx = g_mainCanvas.getContext("2d");
            var blits = g_script[index][2]
            for (var i = 0; i < blits.length; ++i) {
                var blit = blits[i];
                var sx = blit[0];
                var sy = blit[1];
                var w = blit[2];
                var h = blit[3];
                var dx = blit[4];
                var dy = blit[5];
                ctx.drawImage(g_blitImage, sx, sy, w, h, dx, dy, w, h);
            }
        } else if (stepKind == "screen") {
            var url = g_scriptDir + "/" + g_script[index][2];
            var ctx = g_mainCanvas.getContext("2d");
            ctx.clearRect(0, 0, g_mainCanvas.width, g_mainCanvas.height);
            ctx.drawImage(g_imageCache[url], 0, 0);
        } else if (stepKind == "cpos") {
            g_cursorCanvas.style.left = g_script[index][2] + "px";
            g_cursorCanvas.style.top = g_script[index][3] + "px";
        } else if (stepKind == "cimg") {
            var url = g_scriptDir + "/" + g_script[index][2];
            var ctx = g_cursorCanvas.getContext("2d");
            ctx.clearRect(0, 0, g_cursorCanvas.width, g_cursorCanvas.height);
            ctx.drawImage(g_imageCache[url], 0, 0);
        } else {
            alert("Invalid step in animation script: " + g_script[index]);
        }
    }

    function beginCurrentStep()
    {
        assert(!g_paused);
        var delayMS = g_script[g_index][0];
        g_stepTimeoutID = window.setTimeout(function() {
            g_stepTimeoutID = null;
            executeStep(g_index);
            g_index++;
            if (g_index == g_script.length) {
                g_index = 0;
                that.onloop();
                if (g_paused)
                    return;
            }
            beginCurrentStep();
        }, delayMS);
    }

    this.start = function() {
        g_paused = false;
        if (g_loaded && g_stepTimeoutID === null) {
            beginCurrentStep();
        }
    }

    this.pause = function() {
        g_paused = true;
        if (g_stepTimeoutID !== null) {
            assert(g_loaded);
            window.clearTimeout(g_stepTimeoutID);
            g_stepTimeoutID = null;
        }
    }

    this.isPaused = function() {
        return g_paused;
    }

    // Note that this function returns false for the duration of the onload
    // callback.
    this.isLoaded = function() {
        return g_loaded;
    }

    var widthPx = script["width"];
    var heightPx = script["height"];
    g_divElement = document.createElement("div");
    g_divElement.style.position = "relative";
    g_divElement.style.width = widthPx + "px";
    g_divElement.style.height = heightPx + "px";
    g_divElement.style.overflow = "hidden";
    this.element = g_divElement;
    g_mainCanvas = document.createElement("canvas");
    g_mainCanvas.style.position = "absolute";
    g_mainCanvas.style.left = "0px";
    g_mainCanvas.style.top = "0px";
    g_mainCanvas.width = widthPx;
    g_mainCanvas.height = heightPx;
    g_divElement.appendChild(g_mainCanvas);
    g_cursorCanvas = document.createElement("canvas");
    g_cursorCanvas.style.position = "absolute";
    g_cursorCanvas.width = kMaxCursorWidth;
    g_cursorCanvas.height = kMaxCursorHeight;
    g_divElement.appendChild(g_cursorCanvas);

    g_scriptDir = scriptDir
    g_script = script["steps"]
    for (var i = 0; i < g_script.length; ++i) {
        var stepKind = g_script[i][1];
        if (stepKind == "blitimg" ||
                stepKind == "screen" ||
                stepKind == "cimg") {
            g_imageCount += 1;
            var url = g_scriptDir + "/" + g_script[i][2];
            var image = new Image();
            g_imageCache[url] = image;
            image.onload = imageLoaded;
            image.src = url;
        }
    }
}
