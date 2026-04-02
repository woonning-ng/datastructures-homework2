#include "SpatialGrid.hpp"

#include <limits>

void SpatialGrid::Build(const std::map<int, Vertex*>& ring_heads) {
    double lo_x = std::numeric_limits<double>::max();
    double lo_y = std::numeric_limits<double>::max();
    double hi_x = -std::numeric_limits<double>::max();
    double hi_y = -std::numeric_limits<double>::max();
    std::size_t total = 0;

    for (const auto& [ring_id, head] : ring_heads) {
        (void)ring_id;
        if (!head) {
            continue;
        }

        Vertex* v = head;
        do {
            lo_x = std::min(lo_x, v->x);
            hi_x = std::max(hi_x, v->x);
            lo_y = std::min(lo_y, v->y);
            hi_y = std::max(hi_y, v->y);
            ++total;
            v = v->next;
        } while (v && v != head);
    }

    if (total == 0) {
        cells.clear();
        cols = 1;
        rows = 1;
        cell_w = 1.0;
        cell_h = 1.0;
        min_x = 0.0;
        min_y = 0.0;
        query_epoch = 0;
        return;
    }

    const double eps = 1.0;
    min_x = lo_x - eps;
    min_y = lo_y - eps;
    const double width = (hi_x - lo_x) + 2.0 * eps;
    const double height = (hi_y - lo_y) + 2.0 * eps;

    const int grid_dim = std::max(1, static_cast<int>(std::sqrt(static_cast<double>(total))));
    cols = grid_dim;
    rows = grid_dim;
    cell_w = width / cols;
    cell_h = height / rows;
    if (cell_w < 1e-12) cell_w = 1.0;
    if (cell_h < 1e-12) cell_h = 1.0;

    cells.clear();
    cells.resize(static_cast<std::size_t>(cols) * rows);
    query_epoch = 0;

    for (const auto& [ring_id, head] : ring_heads) {
        if (!head) {
            continue;
        }
        Vertex* v = head;
        do {
            Insert(v, ring_id);
            v = v->next;
        } while (v && v != head);
    }
}

void SpatialGrid::Insert(Vertex* v, int ring_id) {
    if (!v || !v->next) {
        return;
    }
    Vertex* w = v->next;
    const double x0 = std::min(v->x, w->x);
    const double y0 = std::min(v->y, w->y);
    const double x1 = std::max(v->x, w->x);
    const double y1 = std::max(v->y, w->y);

    int cx0 = 0;
    int cy0 = 0;
    int cx1 = 0;
    int cy1 = 0;
    CellRange(x0, y0, x1, y1, cx0, cy0, cx1, cy1);

    for (int cy = cy0; cy <= cy1; ++cy) {
        for (int cx = cx0; cx <= cx1; ++cx) {
            const int idx = CellIndex(cx, cy);
            cells[static_cast<std::size_t>(idx)].push_back({v, ring_id});
        }
    }
}

void SpatialGrid::Remove(Vertex* v) {
    if (!v || !v->next) {
        return;
    }
    Vertex* w = v->next;
    const double x0 = std::min(v->x, w->x);
    const double y0 = std::min(v->y, w->y);
    const double x1 = std::max(v->x, w->x);
    const double y1 = std::max(v->y, w->y);

    int cx0 = 0;
    int cy0 = 0;
    int cx1 = 0;
    int cy1 = 0;
    CellRange(x0, y0, x1, y1, cx0, cy0, cx1, cy1);

    for (int cy = cy0; cy <= cy1; ++cy) {
        for (int cx = cx0; cx <= cx1; ++cx) {
            const int idx = CellIndex(cx, cy);
            auto& cell = cells[static_cast<std::size_t>(idx)];
            for (std::size_t i = 0; i < cell.size(); ++i) {
                if (cell[i].start == v) {
                    cell[i] = cell.back();
                    cell.pop_back();
                    break;
                }
            }
        }
    }
}

