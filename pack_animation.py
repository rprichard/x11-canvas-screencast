#!/usr/bin/python
# Copyright (c) 2012, Sublime HQ Pty Ltd
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import scipy.ndimage.measurements as me
import json
import scipy.misc as misc
import re
import sys
import os
import cv2
from cStringIO import StringIO
from numpy import *
from time import time

# How many pixels can be wasted in the name of combining neighbouring changed
# regions.
SIMPLIFICATION_TOLERANCE = 512

MAX_PACKED_HEIGHT = 10000

def slice_size(a, b):
    return (a.stop - a.start) * (b.stop - b.start)

def combine_slices(a, b, c, d):
    return (slice(min(a.start, c.start), max(a.stop, c.stop)),
        slice(min(b.start, d.start), max(b.stop, d.stop)))

def slices_intersect(a, b, c, d):
    if (a.start >= c.stop): return False
    if (c.start >= a.stop): return False
    if (b.start >= d.stop): return False
    if (d.start >= b.stop): return False
    return True

# Combine a large set of rectangles into a smaller set of rectangles,
# minimising the number of additional pixels included in the smaller set of
# rectangles
def simplify(boxes, tol = 0):
    out = []
    for a,b in boxes:
        sz1 = slice_size(a, b)
        did_combine = False
        for i in xrange(len(out)):
            c,d = out[i]
            cu, cv = combine_slices(a, b, c, d)
            sz2 = slice_size(c, d)
            if slices_intersect(a, b, c, d) or (slice_size(cu, cv) <= sz1 + sz2 + tol):
                out[i] = (cu, cv)
                did_combine = True
                break
        if not did_combine:
            out.append((a,b))

    if tol != 0:
        return simplify(out, 0)
    else:
        return out

def slice_tuple_size(s):
    a, b = s
    return (a.stop - a.start) * (b.stop - b.start)

# Allocates space in the packed image. This does it in a slow, brute force
# manner.
class Allocator2D:
    def __init__(self, rows, cols):
        self.bitmap = zeros((rows, cols), dtype=uint8)
        self.available_space = zeros(rows, dtype=uint32)
        self.available_space[:] = cols
        self.num_used_rows = 0

    def allocate(self, w, h):
        bh, bw = shape(self.bitmap)

        for row in xrange(bh - h + 1):
            if self.available_space[row] < w:
                continue

            for col in xrange(bw - w + 1):
                if self.bitmap[row, col] == 0:
                    if not self.bitmap[row:row+h,col:col+w].any():
                        self.bitmap[row:row+h,col:col+w] = 1
                        self.available_space[row:row+h] -= w
                        self.num_used_rows = max(self.num_used_rows, row + h)
                        return row, col
        raise RuntimeError()

def find_matching_rect(bitmap, num_used_rows, packed, src, sx, sy, w, h):
    template = src[sy:sy+h, sx:sx+w]
    bh, bw = shape(bitmap)
    image = packed[0:num_used_rows, 0:bw]

    if num_used_rows < h:
        return None

    result = cv2.matchTemplate(image,template,cv2.TM_CCOEFF_NORMED)

    row,col = unravel_index(result.argmax(),result.shape)
    if ((packed[row:row+h,col:col+w] == src[sy:sy+h,sx:sx+w]).all()
        and (packed[row:row+1,col:col+w,0] == src[sy:sy+1,sx:sx+w,0]).all()):
        return row,col
    else:
        return None

# The script file is a JavaScript file consisting of a single variable
# assignment:
#    script_var = Extended-JSON;
# Extended-JSON is a JSON string, except that:
#  - It may have block (/**/) or line (//) comments.
#  - Trailing commas are permitted.
# This function strips the comments and trailing commas out of the
# Extended-JSON text and parses it to a Python object, which is returned.
def parse_script_file(input):
    output = StringIO()
    quoted = False
    lineComment = False
    blockComment = False
    comma = False
    i = 0
    while i < len(input):
        if lineComment:
            if input[i] == '\n':
                lineComment = False
            i += 1
        elif blockComment:
            if input[i:i+2] == "*/":
                blockComment = False
                i += 2
            else:
                i += 1
        elif quoted:
            if input[i] == "\\" and i + 1 < len(input):
                output.write(input[i:i+2])
                i += 2
            else:
                quoted = (input[i] != '"')
                output.write(input[i])
                i += 1
        else:
            if input[i:i+2] == "//":
                lineComment = True
                i += 2
            elif input[i:i+2] == "/*":
                blockComment = True
                i += 2
            elif input[i].isspace():
                # Strip whitespace.
                i += 1
            elif input[i] == ",":
                # Recognize and strip off trailing commas because the JSON
                # parser rejects them.
                comma = True
                i += 1
            elif input[i] in "]}":
                output.write(input[i])
                i += 1
                comma = False
            else:
                if comma:
                    output.write(",")
                    comma = False
                quoted = (input[i] == '"')
                output.write(input[i])
                i += 1
    assert not quoted and not blockComment
    m = re.match(r"^[A-Za-z0-9_]+=(.*);$", output.getvalue())
    return json.loads(m.group(1))

def read_script(script_filename):
    with open(script_filename) as f:
        return parse_script_file(f.read())

def generate_animation(script_filename):
    assert script_filename.endswith(".js")
    anim_name = script_filename[:-3]
    script = read_script(script_filename)
    frames = [(index, item[2])
                for (index, item) in enumerate(script["steps"])
                if item[1] == "screen"]

    script_dir = os.path.dirname(script_filename)
    images = [misc.imread(os.path.join(script_dir, f)) for i, f in frames]

    zero = images[0] - images[0]
    pairs = zip([zero] + images[:-1], images)
    diffs = [sign((b - a).max(2)) for a, b in pairs]

    # Find different objects for each frame
    img_areas = [me.find_objects(me.label(d)[0]) for d in diffs]

    # Simplify areas
    img_areas = [simplify(x, SIMPLIFICATION_TOLERANCE) for x in img_areas]

    ih, iw, _ = shape(images[0])

    # Generate a packed image
    allocator = Allocator2D(MAX_PACKED_HEIGHT, iw)
    packed = zeros((MAX_PACKED_HEIGHT, iw, 3), dtype=uint8)

    # Sort the rects to be packed by largest size first, to improve the packing
    rects_by_size = []
    for i in xrange(len(images)):
        src_rects = img_areas[i]

        for j in xrange(len(src_rects)):
            rects_by_size.append((slice_tuple_size(src_rects[j]), i, j))

    rects_by_size.sort(reverse = True)

    allocs = [[None] * len(src_rects) for src_rects in img_areas]

    print anim_name,"packing, num rects:",len(rects_by_size),"num frames:",len(images)

    t0 = time()

    for size,i,j in rects_by_size:
        src = images[i]
        src_rects = img_areas[i]

        a, b = src_rects[j]
        sx, sy = b.start, a.start
        w, h = b.stop - b.start, a.stop - a.start

        # See if the image data already exists in the packed image. This takes
        # a long time, but results in worthwhile space savings (20% in one
        # test)
        existing = find_matching_rect(allocator.bitmap, allocator.num_used_rows, packed, src, sx, sy, w, h)
        if existing:
            dy, dx = existing
            allocs[i][j] = (dy, dx)
        else:
            dy, dx = allocator.allocate(w, h)
            allocs[i][j] = (dy, dx)

            packed[dy:dy+h, dx:dx+w] = src[sy:sy+h, sx:sx+w]

    print anim_name,"packing finished, took:",time() - t0

    packed = packed[0:allocator.num_used_rows]

    misc.imsave(anim_name + "_packed_tmp.png", packed)
    os.system("pngcrush -q " + anim_name + "_packed_tmp.png " + anim_name + "_packed.png")
    os.system("rm " + anim_name + "_packed_tmp.png")

    # Generate JSON to represent the data
    new_steps = list(script["steps"])
    for i in xrange(len(images)):
        script_index = frames[i][0]
        src_rects = img_areas[i]
        dst_rects = allocs[i]

        blitlist = []

        for j in xrange(len(src_rects)):
            a, b = src_rects[j]
            sx, sy = b.start, a.start
            w, h = b.stop - b.start, a.stop - a.start
            dy, dx = dst_rects[j]

            blitlist.append(
                [int(dx), int(dy), int(w), int(h), int(sx), int(sy)])

        new_steps[script_index] = [script["steps"][script_index][0], "blit", blitlist]

    packed_png_relpath = os.path.relpath(anim_name + "_packed.png", script_dir)
    new_steps = [[0, "blitimg", packed_png_relpath]] + new_steps
    f = open(anim_name + "_packed.js", "wb")
    f.write(os.path.basename(anim_name) + "_packed = {\n")
    f.write('"width" : %d,\n' % script["width"])
    f.write('"height" : %d,\n' % script["height"])
    f.write('"steps" : [\n')
    for item in new_steps:
        f.write(json.dumps(item) + ",\n")
    f.write("]};\n")
    f.close()


if __name__ == '__main__':
    generate_animation(sys.argv[1])
