"""Split-sum environment BRDF table (our own code).

Following B. Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013 course "Physically Based Shading in
Theory and Practice": the specular image-based-lighting integral is split into the prefiltered environment
times an environment BRDF that depends only on n.v and roughness:

    integral( f(l,v) cos(theta_l) dl ) = F0 * A + B, with, for N importance samples of the GGX lobe,
    A = 1/N sum (1 - Fc) G_vis,   B = 1/N sum Fc G_vis,
    Fc = (1 - v.h)^5 (Schlick),   G_vis = G(n.v, n.l) * v.h / (n.h * n.v),
    G = G1(n.v) G1(n.l), G1(x) = x / (x (1 - k) + k), k = alpha / 2 (Karis' choice for IBL), alpha = roughness^2,
    h drawn from GGX (Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"):
      phi = 2 pi xi1, cos(theta_h) = sqrt((1 - xi2) / (1 + (alpha^2 - 1) xi2)),
    xi from the Hammersley point set (radical inverse in base 2).

H9's shader (glsl/lightmodel.h, ComputeSpecularIBL) reads texture(u_BRDFApproxMap, vec2(n.v, roughness)).rg
with roughness = (1 - smoothness)^2, and uses F0 * r + g. So: x = n.v, y = roughness, R = A, G = B.
"""
import numpy as np


def hammersley(n):
    i = np.arange(n, dtype=np.uint32)
    b = i.copy()
    b = ((b << 16) | (b >> 16)) & 0xFFFFFFFF
    b = ((b & 0x55555555) << 1) | ((b & 0xAAAAAAAA) >> 1)
    b = ((b & 0x33333333) << 2) | ((b & 0xCCCCCCCC) >> 2)
    b = ((b & 0x0F0F0F0F) << 4) | ((b & 0xF0F0F0F0) >> 4)
    b = ((b & 0x00FF00FF) << 8) | ((b & 0xFF00FF00) >> 8)
    return (i + 0.5) / n, b.astype(np.float64) / 4294967296.0


def integrate(nov, rough, samples=4096):
    """nov, rough: 1-D arrays of equal length. Returns (A, B)."""
    nov = np.maximum(np.asarray(nov, np.float64), 1e-4)
    alpha = np.asarray(rough, np.float64) ** 2
    k = alpha / 2.0
    vx, vz = np.sqrt(1.0 - nov * nov), nov                       # v in the xz plane, n = +z
    x1, x2 = hammersley(samples)
    phi = 2.0 * np.pi * x1[None, :]
    a2 = (alpha * alpha)[:, None]
    ct = np.sqrt((1.0 - x2[None, :]) / (1.0 + (a2 - 1.0) * x2[None, :]))
    st = np.sqrt(1.0 - ct * ct)
    hx, hz = st * np.cos(phi), ct                                # hy does not enter v.h (vy = 0)
    voh = vx[:, None] * hx + vz[:, None] * hz
    lz = 2.0 * voh * hz - vz[:, None]                            # n.l
    noh = hz
    ok = lz > 0
    nl = np.where(ok, lz, 1.0)
    g1 = lambda x, kk: x / (x * (1.0 - kk) + kk)
    G = g1(nov[:, None], k[:, None]) * g1(nl, k[:, None])
    vh = np.clip(voh, 0.0, 1.0)
    gvis = np.where(ok, G * vh / (noh * nov[:, None]), 0.0)
    fc = (1.0 - vh) ** 5
    return ((1.0 - fc) * gvis).mean(1), (fc * gvis).mean(1)


def table(size=256, samples=4096, chunk=2048):
    c = (np.arange(size) + 0.5) / size
    nv, rr = np.meshgrid(c, c)            # row = roughness, column = n.v
    nv, rr = nv.ravel(), rr.ravel()
    A = np.empty_like(nv)
    B = np.empty_like(nv)
    for s in range(0, len(nv), chunk):
        A[s:s + chunk], B[s:s + chunk] = integrate(nv[s:s + chunk], rr[s:s + chunk], samples)
    return np.stack([A, B], -1).reshape(size, size, 2)
