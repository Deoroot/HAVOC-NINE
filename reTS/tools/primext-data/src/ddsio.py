"""Minimal DDS writer/reader helpers for the H9 PrimeXT data files (our own code).

Writes the classic 124-byte DDS header (plus the 20-byte DX10 extension when asked), with the
field values the Xash3D image loader needs. References: Microsoft, "DDS_HEADER structure" and
"DDS_HEADER_DXT10 structure" (Direct3D documentation).
"""
import struct

DDSD_CAPS, DDSD_HEIGHT, DDSD_WIDTH, DDSD_PITCH = 0x1, 0x2, 0x4, 0x8
DDSD_PIXELFORMAT, DDSD_MIPMAPCOUNT, DDSD_LINEARSIZE = 0x1000, 0x20000, 0x80000
DDSCAPS_ALPHA, DDSCAPS_COMPLEX, DDSCAPS_TEXTURE, DDSCAPS_MIPMAP = 0x2, 0x8, 0x1000, 0x400000
DDPF_ALPHA, DDPF_FOURCC = 0x2, 0x4


def header(width, height, mips, pf_flags, fourcc=b"\0\0\0\0", bitcount=0, amask=0,
           linear_size=0, extra_caps=0, dx10=None):
    flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT
    if linear_size:
        flags |= DDSD_LINEARSIZE
    caps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | extra_caps
    pf = struct.pack("<II4sIIIII", 32, pf_flags, fourcc, bitcount, 0, 0, 0, amask)
    h = struct.pack("<4sIIIIIII", b"DDS ", 124, flags, height, width, linear_size, 0, mips)
    h += b"\0" * 44 + pf + struct.pack("<IIIII", caps, 0, 0, 0, 0)
    assert len(h) == 128
    if dx10 is not None:
        dxgi, dim, alpha_mode = dx10
        h += struct.pack("<IIIII", dxgi, dim, 0, 1, alpha_mode)
    return h


def mip_sizes(w, h):
    out = []
    while True:
        out.append((w, h))
        if w == 1 and h == 1:
            return out
        w, h = max(1, w // 2), max(1, h // 2)
