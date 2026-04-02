#ifndef SPATIAL_GRID_HPP
#define SPATIAL_GRID_HPP

#include "Polygon.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

struct Segment {
    Vertex* start = nullptr;
    int ring_id = -1;
};

class SpatialGrid {
public:
    SpatialGrid()
        : min_x(0.0),
          min_y(0.0),
          cell_w(1.0),
          cell_h(1.0),
          cols(1),
          rows(1),
          query_epoch(0) {}

    void Build(const std::map<int, Vertex*>& ring_heads);
    void Insert(Vertex* v, int ring_id);
    void Remove(Vertex* v);

    template <typename Func>
    void Query(double qx0, double qy0, double qx1, double qy1, Func&& visitor) const {
        int cx0 = 0;
        int cy0 = 0;
        int cx1 = 0;
        int cy1 = 0;
        CellRange(qx0, qy0, qx1, qy1, cx0, cy0, cx1, cy1);

        ++query_epoch;
        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                const int idx = CellIndex(cx, cy);
                for (const Segment& seg : cells[static_cast<std::size_t>(idx)]) {
                    if (seg.start && seg.start->last_query_epoch != query_epoch) {
                        seg.start->last_query_epoch = query_epoch;
                        if (!visitor(seg.start, seg.ring_id)) {
                            return;
                        }
                    }
                }
            }
        }
    }

private:
    double min_x;
    double min_y;
    double cell_w;
    double cell_h;
    int cols;
    int rows;

    std::vector<std::vector<Segment>> cells;
    mutable unsigned long long query_epoch;

    int CellIndex(int cx, int cy) const { return cy * cols + cx; }

    void CellRange(
        double x0,
        double y0,
        double x1,
        double y1,
        int& cx0,
        int& cy0,
        int& cx1,
        int& cy1
    ) const {
        const double ax0 = std::min(x0, x1);
        const double ay0 = std::min(y0, y1);
        const double ax1 = std::max(x0, x1);
        const double ay1 = std::max(y0, y1);

        cx0 = std::max(0, std::min(cols - 1, static_cast<int>((ax0 - min_x) / cell_w)));
        cy0 = std::max(0, std::min(rows - 1, static_cast<int>((ay0 - min_y) / cell_h)));
        cx1 = std::max(0, std::min(cols - 1, static_cast<int>((ax1 - min_x) / cell_w)));
        cy1 = std::max(0, std::min(rows - 1, static_cast<int>((ay1 - min_y) / cell_h)));
    }
};

#endif
