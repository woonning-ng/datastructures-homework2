#!/usr/bin/env python3
import argparse
import csv
from pathlib import Path
import json
def write_csv(path: Path, points):
    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["ring_id", "vertex_id", "x", "y"])
        for i, (x, y) in enumerate(points):
            writer.writerow([0, i, f"{x:.6f}", f"{y:.6f}"])

def make_comb(teeth: int, tooth_width: float = 1.0, gap_width: float = 1.0, height: float = 2.0, notch_depth: float = 1.0):
    # Creates a single exterior ring shaped like a comb.
    # Bottom edge from (0,0) to (W,0), then up right edge, then zig-zag across the top.
    width = teeth * tooth_width + (teeth - 1) * gap_width
    pts = [(0.0, 0.0), (width, 0.0), (width, height)]

    x = width
    for t in range(teeth - 1, -1, -1):
        tooth_left = t * (tooth_width + gap_width)
        tooth_right = tooth_left + tooth_width
        if abs(x - tooth_right) > 1e-12:
            pts.append((tooth_right, height))
        pts.append((tooth_right, height - notch_depth))
        pts.append((tooth_left, height - notch_depth))
        pts.append((tooth_left, height))

    pts.append((0.0, 0.0))
    # remove duplicate closing point because project CSV should not repeat first vertex at end
    if pts[-1] == pts[0]:
        pts.pop()
    return pts

def main():
    parser = argparse.ArgumentParser(description="Generate scalable comb test cases for polygon simplification.")
    parser.add_argument("--teeth", nargs="+", type=int, default=[5, 10, 20, 40, 80], help="List of tooth counts")
    parser.add_argument("--out-dir", default="generated_cases", help="Output directory")
    parser.add_argument("--tooth-width", type=float, default=1.0)
    parser.add_argument("--gap-width", type=float, default=1.0)
    parser.add_argument("--height", type=float, default=2.0)
    parser.add_argument("--notch-depth", type=float, default=1.0)
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest = {"scaling_cases": []}
    for teeth in args.teeth:
        pts = make_comb(teeth, args.tooth_width, args.gap_width, args.height, args.notch_depth)
        out_path = out_dir / f"test_comb_teeth_{teeth}.csv"
        write_csv(out_path, pts)
        manifest["scaling_cases"].append({
            "file": str(out_path),
            "teeth": teeth,
            "vertices": len(pts),
            "description": f"Comb exterior with {teeth} teeth for scaling experiments"
        })
        print(f"Wrote {out_path} ({len(pts)} vertices)")

    manifest_path = out_dir / "scaling_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))
    print(f"Wrote {manifest_path}")

if __name__ == "__main__":
    main()
