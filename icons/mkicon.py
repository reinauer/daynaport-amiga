#!/usr/bin/env python3
# mkicon.py - writes the Workbench icons for the installer archive:
#   icons/mkicon.py icons/Install-scsidayna.info icons/drawer.info
# The project icon (Install-scsidayna) starts the Installer; the drawer icon
# makes the unpacked "scsidayna" drawer show up in Workbench. Both use the
# 4-colour Workbench 2/3 palette: 0 grey, 1 black, 2 white, 3 blue.
import struct, math, sys
W, H = 40, 26
img = [[0]*W for _ in range(H)]
# Wi-Fi arcs (blue): per row, the |x - centre| ranges that are blue
arcs = {
    1: [(0, 9)], 2: [(7.5, 12)], 3: [(10.5, 14)], 4: [(0, 5.5), (12.5, 15.5)], 5: [(4.5, 8), (14, 16.5)],
    6: [(7, 9.5)], 7: [(0, 2.5), (8.5, 10.5)], 8: [(1.5, 4.5)], 9: [(3.5, 5.5)],
    11: [(0, 1)], 12: [(0, 1)],
}
for y, ranges in arcs.items():
    for x in range(W):
        d = abs(x - 19.5)
        if any(lo <= d <= hi for lo, hi in ranges): img[y][x] = 3
# the adapter: a box with a bevel, slots and a blue LED
x0, x1, y0, y1 = 6, 33, 15, 24
for y in range(y0, y1 + 1):
    for x in range(x0, x1 + 1):
        if y == y0 or x == x0: img[y][x] = 2
        elif y == y1 or x == x1: img[y][x] = 1
        else: img[y][x] = 0
for x in range(x0 + 3, x0 + 19, 2):
    for y in range(y0 + 3, y1 - 2): img[y][x] = 1
for y in (y0 + 4, y0 + 5):
    for x in (x1 - 5, x1 - 4): img[y][x] = 3

def planes(img):
    rb = ((W + 15) // 16) * 2
    out = b""
    for p in range(2):
        for row in img:
            bits = bytearray(rb)
            for x, c in enumerate(row):
                if c >> p & 1: bits[x // 8] |= 0x80 >> (x % 8)
            out += bytes(bits)
    return out

def s(t):
    b = t.encode("latin-1") + b"\0"
    return struct.pack(">L", len(b)) + b

tooltypes = ["APPNAME=scsidayna.device", "MINUSER=AVERAGE"]
gadget = struct.pack(">LhhhhHHHLLLlLHL", 0, 0, 0, W, H + 1, 0x0005, 0x0003, 0x0001,
                     1, 0, 0, 0, 0, 0, 1)
do = struct.pack(">HH", 0xE310, 1) + gadget + struct.pack(">BBLLllLLl",
     4, 0, 1, 1, -0x80000000, -0x80000000, 0, 0, 16384)
assert len(do) == 78
image = struct.pack(">hhhhhLBBL", 0, 0, W, H, 2, 1, 3, 0, 0)
data = do + image + planes(img) + s("Installer")
data += struct.pack(">L", (len(tooltypes) + 1) * 4) + b"".join(s(t) for t in tooltypes)
open(sys.argv[1], "wb").write(data)

# ---- drawer icon (OS 1.3 style: no DrawerData2 extension) ----
if len(sys.argv) > 2:
    DW, DH = 40, 18
    d = [[0]*DW for _ in range(DH)]
    x0, x1, y0, y1 = 3, 36, 2, 16
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if y == y0 or x == x0: d[y][x] = 2
            elif y == y1 or x == x1: d[y][x] = 1
    for x in range(x0 + 2, x1 - 1):                 # drawer front
        d[y0 + 6][x] = 1
        d[y0 + 7][x] = 2
    for y in range(y0 + 2, y0 + 5):                  # handle
        for x in range(16, 24): d[y][x] = 3 if y == y0 + 3 else 1
    W, H = DW, DH
    gadget = struct.pack(">LhhhhHHHLLLlLHL", 0, 0, 0, W, H + 1, 0x0005, 0x0003, 0x0001,
                         1, 0, 0, 0, 0, 0, 0)
    do = struct.pack(">HH", 0xE310, 1) + gadget + struct.pack(">BBLLllLLl",
         2, 0, 0, 0, -0x80000000, -0x80000000, 1, 0, 0)
    newwin = struct.pack(">hhhhBBLLLLLLLhhHHH", 60, 40, 420, 140, 255, 255, 0, 0,
                         0, 0, 0, 0, 0, 90, 40, 0xFFFF, 0xFFFF, 1)
    assert len(newwin) == 48
    drawer = newwin + struct.pack(">ll", 0, 0)
    image = struct.pack(">hhhhhLBBL", 0, 0, W, H, 2, 1, 3, 0, 0)
    open(sys.argv[2], "wb").write(do + drawer + image + planes(d))
