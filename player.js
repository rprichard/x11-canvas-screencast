var g_script = null;
var g_index = -1;
var g_mainCanvas = null;
var g_cursorCanvas = null;

var g_blitImage = null;

var kMaxCursorWidth = 32;
var kMaxCursorHeight = 32;

var g_imageCache = {};
var g_imageCount = 0;
var g_imageLoadCount = 0;

function width()
{
    return g_mainCanvas.width;
}

function height()
{
    return g_mainCanvas.height;
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

function player(parent, scriptUrl, widthPx, heightPx)
{
    var div = document.createElement("div");
    div.style.position = "relative";
    div.style.width = widthPx + "px";
    div.style.height = heightPx + "px";
    div.style.overflow = "hidden";
    parent.appendChild(div);
    g_mainCanvas = document.createElement("canvas");
    g_mainCanvas.style.position = "absolute";
    g_mainCanvas.width = widthPx;
    g_mainCanvas.height = heightPx;
    div.appendChild(g_mainCanvas);
    g_cursorCanvas = document.createElement("canvas");
    g_cursorCanvas.style.position = "absolute";
    g_cursorCanvas.width = kMaxCursorWidth;
    g_cursorCanvas.height = kMaxCursorHeight;
    div.appendChild(g_cursorCanvas);

    get(scriptUrl, function(script) {
        g_script = eval("[\n" + script + "\n]");
        for (var i = 0; i < g_script.length; ++i) {
            var stepKind = g_script[i][1];
            if (stepKind == "blitimg" ||
                    stepKind == "screen" ||
                    stepKind == "cimg") {
                g_imageCount += 1;
                var url = g_script[i][2];
                var image = new Image();
                g_imageCache[url] = image;
                image.onload = imageLoaded;
                image.src = url;
            }
        }
    });
}

function imageLoaded()
{
    g_imageLoadCount++;
    if (g_imageLoadCount == g_imageCount) {
        g_index = 0;
        executeCurrentStep();
    }
}

function executeCurrentStep()
{
    var stepKind = g_script[g_index][1];
    if (stepKind == "blitimg") {
        var url = g_script[g_index][2];
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
        var url = g_script[g_index][2];
        var ctx = g_mainCanvas.getContext("2d");
        ctx.clearRect(0, 0, width(), height());
        ctx.drawImage(g_imageCache[url], 0, 0);
        advanceToNextStep();
    } else if (stepKind == "cpos") {
        g_cursorCanvas.style.left = g_script[g_index][2] + "px";
        g_cursorCanvas.style.top = g_script[g_index][3] + "px";
        advanceToNextStep();
    } else if (stepKind == "cimg") {
        var url = g_script[g_index][2];
        var ctx = g_cursorCanvas.getContext("2d");
        ctx.clearRect(0, 0, kMaxCursorWidth, kMaxCursorHeight);
        ctx.drawImage(g_imageCache[url], 0, 0);
        advanceToNextStep();
    } else {
        alert("Invalid step in animation script: " + g_script[g_index]);
    }
}

function advanceToNextStep()
{
    g_index = (g_index + 1) % g_script.length;
    var delayMS = g_script[g_index][0];
    window.setTimeout(executeCurrentStep, delayMS);
}
