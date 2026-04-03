#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path

def write_csv(path, points):
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["ring_id", "vertex_id", "x", "y"])
        for i, (x, y) in enumerate(points):
            writer.writerow([0, i, x, y])

def make_comb(teeth):
    width = teeth * 2
    pts = [(0,0), (width,0), (width,2)]
    x = width

    for t in range(teeth-1, -1, -1):
        left = t * 2
        right = left + 1

        if x != right:
            pts.append((right,2))

        pts.append((right,1))
        pts.append((left,1))
        pts.append((left,2))

        x = left

    pts.append((0,0))
    if pts[-1] == pts[0]:
        pts.pop()

    return pts

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", default="our_test_cases")
    parser.add_argument("--teeth", nargs="+", type=int,
                        default=[50,100,200,400,800,1600])
    args = parser.parse_args()

    out = Path(args.out_dir)
    out.mkdir(exist_ok=True)

    manifest = {"cases":[]}

    for t in args.teeth:
        pts = make_comb(t)
        path = out / f"test_comb_teeth_{t}.csv"
        write_csv(path, pts)

        manifest["cases"].append({
            "file": str(path),
            "teeth": t,
            "vertices": len(pts)
        })

        print(f"Wrote {path} ({len(pts)} vertices)")

    (out / "scaling_manifest.json").write_text(json.dumps(manifest, indent=2))

if __name__ == "__main__":
    main()
