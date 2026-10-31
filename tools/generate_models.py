#!/usr/bin/env python3
"""Generates the plant stage models in assets/models/ (Wavefront OBJ + MTL, no dependencies).

Every part is a closed surface wound counter-clockwise seen from outside, so back-face culling
never removes a visible face. Units are meters; the origin is the center of the soil surface.
Run from the repository root:  python3 tools/generate_models.py
"""
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "models"

MATERIALS = {
    #  name           Kd (diffuse)             Ks     Ns
    "mound":      ((0.36, 0.25, 0.15), 0.05, 8),
    "seed":       ((0.55, 0.38, 0.20), 0.30, 40),
    "stem":       ((0.32, 0.58, 0.22), 0.15, 24),
    "leaf":       ((0.26, 0.64, 0.23), 0.30, 48),
    "leaf_dark":  ((0.16, 0.47, 0.18), 0.30, 48),
    "fruit":      ((0.88, 0.16, 0.10), 0.70, 96),
    "flower":     ((1.00, 0.84, 0.22), 0.20, 24),
}


# --- small vector helpers -------------------------------------------------------------------
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def norm(a):
    l = math.sqrt(dot(a, a))
    return mul(a, 1.0 / l) if l > 1e-12 else (0.0, 1.0, 0.0)


def basis(x_axis, up_hint=(0.0, 1.0, 0.0)):
    """Orthonormal basis whose local x is x_axis; local y is as close to up_hint as possible."""
    x = norm(x_axis)
    if abs(dot(x, norm(up_hint))) > 0.99:
        up_hint = (0.0, 0.0, 1.0)
    z = norm(cross(x, up_hint))
    y = cross(z, x)
    return x, y, z


class Part:
    """Closed grid surface: rows go from pole to pole (or cap to cap), columns wrap around."""

    def __init__(self, material, rows, center):
        self.material = material
        self.rows = rows  # list of rows, each a list of world-space points (same length)
        self.center = center

    def triangles(self):
        rows = self.rows
        cols = len(rows[0])
        for r in range(len(rows) - 1):
            for c in range(cols):
                c2 = (c + 1) % cols
                yield rows[r][c], rows[r][c2], rows[r + 1][c2]
                yield rows[r][c], rows[r + 1][c2], rows[r + 1][c]


def ellipsoid(material, center, radii, x_axis=(1.0, 0.0, 0.0), up=(0.0, 1.0, 0.0),
              slices=18, stacks=12, taper=0.0):
    """Ellipsoid with local radii (rx, ry, rz) along (x_axis, up-ish, side).
    taper > 0 narrows the width towards both tips (leaf shape)."""
    bx, by, bz = basis(x_axis, up)
    rows = []
    for i in range(stacks + 1):
        a = math.pi * i / stacks          # 0..pi from the -x tip to the +x tip
        lx = -math.cos(a)
        ring = math.sin(a)
        width = ring * (1.0 - taper * abs(lx) ** 1.5)
        row = []
        for j in range(slices):
            t = 2.0 * math.pi * j / slices
            local = (lx * radii[0], math.cos(t) * width * radii[1], math.sin(t) * width * radii[2])
            p = add(center, add(add(mul(bx, local[0]), mul(by, local[1])), mul(bz, local[2])))
            row.append(p)
        rows.append(row)
    return Part(material, rows, center)


def tube(material, points, radii, slices=12):
    """Closed tube through points (a stem), with flat-ish caps made of a collapsed end ring."""
    rows = []
    n = len(points)
    for i, (p, r) in enumerate(zip(points, radii)):
        d = sub(points[min(i + 1, n - 1)], points[max(i - 1, 0)])
        _, by, bz = basis(d, (1.0, 0.0, 0.0))
        rows.append([add(p, add(mul(by, math.cos(2 * math.pi * j / slices) * r),
                                mul(bz, math.sin(2 * math.pi * j / slices) * r))) for j in range(slices)])
    rows.insert(0, [points[0]] * slices)   # bottom cap
    rows.append([points[-1]] * slices)     # top cap
    center = mul(add(points[0], points[-1]), 0.5)
    return Part(material, rows, center)


def write_model(name, parts):
    positions, normals, faces = [], [], {}
    for part in parts:
        tris = [t for t in part.triangles() if dot(cross(sub(t[1], t[0]), sub(t[2], t[0])), cross(sub(t[1], t[0]), sub(t[2], t[0]))) > 1e-16]
        # The grid gives every triangle of a part the same orientation; make it counter-clockwise seen
        # from outside by checking the sign of the enclosed volume (divergence theorem).
        volume = sum(dot(sub(a, part.center), cross(sub(b, part.center), sub(c, part.center))) for a, b, c in tris)
        if volume < 0.0:
            tris = [(a, c, b) for a, b, c in tris]
        # Smooth normals: average face normals over coincident positions.
        acc = {}
        key = lambda p: (round(p[0], 6), round(p[1], 6), round(p[2], 6))
        for a, b, c in tris:
            fn = cross(sub(b, a), sub(c, a))
            for p in (a, b, c):
                k = key(p)
                acc[k] = add(acc.get(k, (0.0, 0.0, 0.0)), fn)
        index = {}
        out = faces.setdefault(part.material, [])
        for tri in tris:
            ids = []
            for p in tri:
                k = key(p)
                if k not in index:
                    positions.append(p)
                    normals.append(norm(acc[k]))
                    index[k] = len(positions)
                ids.append(index[k])
            out.append(ids)

    with open(OUT / f"{name}.mtl", "w") as f:
        f.write(f"# {name} plant stage, generated by tools/generate_models.py\n")
        for mat in faces:
            kd, ks, ns = MATERIALS[mat]
            f.write(f"\nnewmtl {mat}\nKd {kd[0]:.3f} {kd[1]:.3f} {kd[2]:.3f}\n"
                    f"Ks {ks:.3f} {ks:.3f} {ks:.3f}\nNs {ns}\n")
    with open(OUT / f"{name}.obj", "w") as f:
        f.write(f"# {name} plant stage, generated by tools/generate_models.py\nmtllib {name}.mtl\n")
        for p in positions:
            f.write(f"v {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}\n")
        for n in normals:
            f.write(f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")
        for mat, tris in faces.items():
            f.write(f"o {mat}\nusemtl {mat}\n")
            for a, b, c in tris:
                f.write(f"f {a}//{a} {b}//{b} {c}//{c}\n")
    print(f"{name}: {len(positions)} vertices, {sum(len(t) for t in faces.values())} triangles")


def leaf(material, attach, direction, length, width, thickness=0.005):
    d = norm(direction)
    return ellipsoid(material, add(attach, mul(d, 0.5 * length)), (0.5 * length, thickness, 0.5 * width),
                     x_axis=d, up=(0.0, 1.0, 0.0), slices=14, stacks=12, taper=0.45)


def seed():
    tilt = math.radians(25.0)
    seed_dir = (math.cos(tilt), math.sin(tilt), 0.0)
    return [
        ellipsoid("mound", (0.0, -0.004, 0.0), (0.065, 0.022, 0.065), slices=24, stacks=12),
        ellipsoid("seed", (0.0, 0.024, 0.0), (0.024, 0.014, 0.015), x_axis=seed_dir),
        ellipsoid("leaf", add((0.0, 0.024, 0.0), mul(seed_dir, 0.022)), (0.008, 0.006, 0.006),
                  x_axis=(0.3, 1.0, 0.0), slices=10, stacks=8),
    ]


def sprout():
    top = (0.0, 0.15, 0.0)
    parts = [
        ellipsoid("mound", (0.0, -0.004, 0.0), (0.045, 0.014, 0.045), slices=20, stacks=10),
        tube("stem", [(0.0, -0.01, 0.0), (0.006, 0.07, 0.0), (0.0, 0.15, 0.0)], [0.008, 0.007, 0.0055]),
    ]
    for s in (-1.0, 1.0):
        parts.append(leaf("leaf", top, (s, 0.35, 0.0), 0.08, 0.045))
        parts.append(leaf("leaf_dark", add(top, (0.0, 0.004, 0.0)), (0.0, 1.0, 0.45 * s), 0.045, 0.026))
    return parts


def mature():
    stem_pts = [(0.0, -0.01, 0.0), (0.012, 0.16, 0.004), (-0.004, 0.32, -0.006), (0.004, 0.50, 0.0)]
    parts = [tube("stem", stem_pts, [0.016, 0.013, 0.010, 0.007], slices=14)]

    def stem_at(y):
        # Linear interpolation along the stem polyline by height.
        for a, b in zip(stem_pts, stem_pts[1:]):
            if a[1] <= y <= b[1]:
                t = (y - a[1]) / (b[1] - a[1])
                return add(a, mul(sub(b, a), t))
        return stem_pts[-1]

    golden = math.radians(137.5)
    for i, y in enumerate((0.07, 0.13, 0.19, 0.25, 0.31, 0.37, 0.43)):
        a = i * golden
        t = i / 6.0
        parts.append(leaf("leaf" if i % 2 else "leaf_dark", stem_at(y), (math.cos(a), 0.45 + 0.2 * t, math.sin(a)),
                          0.15 - 0.05 * t, 0.085 - 0.025 * t, 0.006))

    # Fruits hanging from short side stems, with a small calyx on top.
    for y, deg, reach in ((0.17, 40.0, 0.085), (0.27, 175.0, 0.08), (0.35, 290.0, 0.075)):
        a = math.radians(deg)
        base = stem_at(y)
        tip = add(base, (math.cos(a) * reach, -0.01, math.sin(a) * reach))
        parts.append(tube("stem", [base, add(base, mul(sub(tip, base), 0.5)), tip], [0.005, 0.0045, 0.004], slices=8))
        fruit_c = add(tip, (0.0, -0.034, 0.0))
        parts.append(ellipsoid("fruit", fruit_c, (0.036, 0.033, 0.036), x_axis=(0.0, 1.0, 0.0), up=(1.0, 0.0, 0.0),
                               slices=20, stacks=14))
        for k in range(5):
            b = 2.0 * math.pi * k / 5.0
            parts.append(leaf("leaf_dark", add(tip, (0.0, -0.004, 0.0)), (math.cos(b), -0.25, math.sin(b)), 0.028, 0.012, 0.003))

    # Small yellow flowers at the top.
    top = stem_pts[-1]
    for k in range(3):
        b = 2.0 * math.pi * k / 3.0 + 0.4
        c = add(top, (math.cos(b) * 0.03, 0.012 + 0.008 * k, math.sin(b) * 0.03))
        parts.append(ellipsoid("flower", c, (0.016, 0.009, 0.016), x_axis=(1.0, 0.0, 0.0), slices=12, stacks=8))
    return parts


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    write_model("seed", seed())
    write_model("sprout", sprout())
    write_model("mature", mature())


if __name__ == "__main__":
    main()
