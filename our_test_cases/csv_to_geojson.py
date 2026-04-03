#!/usr/bin/env python3
# csv_to_geojson.py
# Converts ring_id,vertex_id,x,y CSV format to GeoJSON
# so it can be pasted into geojson.io to visualise the polygon.
#
# Usage:
#   python3 csv_to_geojson.py input.csv
#   python3 csv_to_geojson.py input.csv output.geojson   (saves to file)

import sys
import json
import csv
from collections import defaultdict

def csv_to_geojson(filepath):
    rings = defaultdict(list)

    with open(filepath, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            ring_id  = int(row['ring_id'])
            x        = float(row['x'])
            y        = float(row['y'])
            rings[ring_id].append((x, y))

    # GeoJSON Polygon: first ring = exterior, rest = holes
    # Each ring must be closed (first point repeated at end)
    coordinates = []
    for ring_id in sorted(rings.keys()):
        ring = rings[ring_id]
        closed = ring + [ring[0]]  # close the ring
        coordinates.append(closed)

    geojson = {
        "type": "FeatureCollection",
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Polygon",
                    "coordinates": coordinates
                },
                "properties": {
                    "source": filepath
                }
            }
        ]
    }
    return geojson

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 csv_to_geojson.py input.csv [output.geojson]")
        sys.exit(1)

    result = csv_to_geojson(sys.argv[1])
    output = json.dumps(result, indent=2)

    if len(sys.argv) >= 3:
        with open(sys.argv[2], 'w') as f:
            f.write(output)
        print(f"Saved to {sys.argv[2]}")
    else:
        print(output)
