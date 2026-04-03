#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path

def write_csv(path, rings):
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["ring_id", "vertex_id", "x", "y"])
        for ring_id, points in enumerate(rings):
            for vid, (x, y) in enumerate(points):
                writer.writerow([ring_id, vid, f"{x:.6f}", f"{y:.6f}"])

def subdivide_segment(p0, p1, subdivisions, include_start=True, include_end=False):
    x0, y0 = p0
    x1, y1 = p1
    pts = []
    start_i = 0 if include_start else 1
    end_i = subdivisions if include_end else subdivisions - 1
    for i in range(start_i, end_i + 1):
        t = i / subdivisions
        pts.append((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t))
    return pts

def subdivide_ring(points, subdivisions):
    out = []
    n = len(points)
    for i in range(n):
        p0 = points[i]
        p1 = points[(i + 1) % n]
        seg = subdivide_segment(p0, p1, subdivisions, include_start=(i == 0), include_end=False)
        out.extend(seg)
    return out

def make_comb(teeth):
    width = teeth * 2.0
    pts = [(0.0, 0.0), (width, 0.0), (width, 2.0)]
    x = width
    for t in range(teeth - 1, -1, -1):
        left = t * 2.0
        right = left + 1.0
        if abs(x - right) > 1e-12:
            pts.append((right, 2.0))
        pts.append((right, 1.0))
        pts.append((left, 1.0))
        pts.append((left, 2.0))
        x = left
    if pts[-1] == pts[0]:
        pts.pop()
    return [pts]

def make_hole_graze(subdivisions):
    outer = [(0.0,0.0), (10.0,0.0), (10.0,10.0), (0.0,10.0)]
    hole = [(5.0,9.9), (9.9,5.0), (5.0,0.1), (0.1,5.0)]
    return [subdivide_ring(outer, subdivisions), subdivide_ring(hole, subdivisions)]

def make_corridor(subdivisions):
    outer = [(0.0,0.0), (20.0,0.0), (20.0,1.0), (0.0,1.0)]
    hole = [(0.5,0.2), (0.5,0.8), (19.5,0.8), (19.5,0.2)]
    return [subdivide_ring(outer, subdivisions), subdivide_ring(hole, subdivisions)]

def make_two_close_holes(subdivisions):
    outer = [(0.0,0.0), (10.0,0.0), (10.0,10.0), (0.0,10.0)]
    hole1 = [(1.0,1.0), (1.0,9.0), (4.95,9.0), (4.95,1.0)]
    hole2 = [(5.05,1.0), (5.05,9.0), (9.0,9.0), (9.0,1.0)]
    return [subdivide_ring(outer, subdivisions), subdivide_ring(hole1, subdivisions), subdivide_ring(hole2, subdivisions)]

def total_vertices(rings):
    return sum(len(r) for r in rings)

def main():
    parser = argparse.ArgumentParser(description="Generate larger scalable CSV families for all polygon test types.")
    parser.add_argument("--out-dir", default="our_test_cases")
    parser.add_argument("--comb-teeth", nargs="+", type=int, default=[50, 100, 200, 400, 800, 1600])
    parser.add_argument("--subdivisions", nargs="+", type=int, default=[20, 40, 80, 160, 320])
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest = {
        "comb": [],
        "hole_graze": [],
        "corridor": [],
        "two_close_holes": [],
    }

    for teeth in args.comb_teeth:
        rings = make_comb(teeth)
        path = out_dir / f"test_comb_teeth_{teeth}.csv"
        write_csv(path, rings)
        manifest["comb"].append({
            "file": str(path),
            "parameter": teeth,
            "vertices": total_vertices(rings),
            "description": f"Comb exterior with {teeth} teeth"
        })
        print(f"Wrote {path} ({total_vertices(rings)} vertices)")

    families = [
        ("hole_graze", make_hole_graze, "test_hole_graze"),
        ("corridor", make_corridor, "test_corridor"),
        ("two_close_holes", make_two_close_holes, "test_twoholes"),
    ]

    for family_name, builder, stem in families:
        for s in args.subdivisions:
            rings = builder(s)
            path = out_dir / f"{stem}_{s}.csv"
            write_csv(path, rings)
            manifest[family_name].append({
                "file": str(path),
                "parameter": s,
                "vertices": total_vertices(rings),
                "description": f"{family_name} with {s} subdivisions per edge"
            })
            print(f"Wrote {path} ({total_vertices(rings)} vertices)")

    manifest_path = out_dir / "all_scaled_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))
    print(f"Wrote {manifest_path}")

if __name__ == "__main__":
    main()
