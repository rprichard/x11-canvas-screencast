function Player(parent, scriptUrl, widthPx, heightPx)
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
    var g_pauseFlag = true;
    var g_paused = false;

    this.onload = function() {};

    // Unlike POSIX dirname, this function's return value always ends with '/'.
    function dirname(path) {
        var ret = path.match(/.*\//);
        if (ret == "")
            ret = "./";
        return ret;
    }

    function get(url, continuation)
    {
        var xmlhttp = new XMLHttpRequest();
        xmlhttp.onreadystatechange = function() {
            if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
                continuation(xmlhttp.responseText);
            }
        }
        xmlhttp.open("GET", url, true);
        xmlhttp.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
        xmlhttp.send(null);
    }

    function imageLoaded()
    {
        g_imageLoadCount++;
        if (g_imageLoadCount == g_imageCount) {
            g_index = -1;
            advanceToNextStep();
        }
    }

    function executeCurrentStep()
    {
        var stepKind = g_script[g_index][1];
        if (stepKind == "blitimg") {
            var url = g_scriptDir + "/" + g_script[g_index][2];
            g_blitImage = g_imageCache[url];
            advanceToNextStep();
        } else if (stepKind == "blit") {
            var ctx = g_mainCanvas.getContext("2d");
            var blits = g_script[g_index][2]
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
            advanceToNextStep();
        } else if (stepKind == "screen") {
            var url = g_scriptDir + "/" + g_script[g_index][2];
            var ctx = g_mainCanvas.getContext("2d");
            ctx.clearRect(0, 0, g_mainCanvas.width, g_mainCanvas.height);
            ctx.drawImage(g_imageCache[url], 0, 0);
            advanceToNextStep();
        } else if (stepKind == "cpos") {
            g_cursorCanvas.style.left = g_script[g_index][2] + "px";
            g_cursorCanvas.style.top = g_script[g_index][3] + "px";
            advanceToNextStep();
        } else if (stepKind == "cimg") {
            var url = g_scriptDir + "/" + g_script[g_index][2];
            var ctx = g_cursorCanvas.getContext("2d");
            ctx.clearRect(0, 0, g_cursorCanvas.width, g_cursorCanvas.height);
            ctx.drawImage(g_imageCache[url], 0, 0);
            advanceToNextStep();
        } else {
            alert("Invalid step in animation script: " + g_script[g_index]);
        }
    }

    function advanceToNextStep()
    {
        g_index = (g_index + 1) % g_script.length;
        waitOnCurrentStep()
    }

    function waitOnCurrentStep()
    {
        var delayMS = g_script[g_index][0];
        if (delayMS != 0) {
            // Delay the onload notification until the first wait.  At this
            // point, the animation script has had an opportunity to paint the
            // canvas, so we can avoid flicker.
            if (!g_loaded) {
                g_loaded = true;
                that.onload();
            }
            // If the player is set to "pause", then avoid setting a timeout.
            // Set g_paused to record the fact that a timeout needs to created
            // if/when start() is called.
            if (g_pauseFlag) {
                g_paused = true;
                return;
            }
        }
        window.setTimeout(executeCurrentStep, delayMS);
    }

    this.start = function() {
        g_pauseFlag = false;
        if (g_paused) {
            g_paused = false;
            waitOnCurrentStep();
        }
    }

    this.pause = function() {
        g_pauseFlag = true;
    }

    g_divElement = document.createElement("div");
    g_divElement.style.position = "relative";
    g_divElement.style.width = widthPx + "px";
    g_divElement.style.height = heightPx + "px";
    g_divElement.style.overflow = "hidden";
    parent.appendChild(g_divElement);
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

    g_scriptDir = dirname(scriptUrl);
    get(scriptUrl, function(script) {
        g_script = eval("[\n" + script + "\n]");
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
    });
}
