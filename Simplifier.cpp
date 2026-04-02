#include "Simplifier.hpp"

#include <cmath>
#include <set>

namespace {

bool IsSameRing(const Vertex* a, const Vertex* b, const Vertex* c, const Vertex* d) {
    return a->ring_id == b->ring_id &&
           b->ring_id == c->ring_id &&
           c->ring_id == d->ring_id;
}

} // namespace

bool CollapseCandidateCompare::operator()(
    const CollapseCandidate& lhs,
    const CollapseCandidate& rhs
) const {
    const double eps = 1e-12;
    const double diff = lhs.collapse.local_displacement - rhs.collapse.local_displacement;
    if (std::fabs(diff) > eps) {
        return diff > 0.0;  // Min-heap: smaller displacement has higher priority
    }

    // Tie-breaking: smaller ring_id first
    if (lhs.a->ring_id != rhs.a->ring_id) {
        return lhs.a->ring_id > rhs.a->ring_id;
    }

    const int lhs_a = lhs.a ? lhs.a->original_vertex_id : -1;
    const int lhs_b = lhs.b ? lhs.b->original_vertex_id : -1;
    const int lhs_c = lhs.c ? lhs.c->original_vertex_id : -1;
    const int lhs_d = lhs.d ? lhs.d->original_vertex_id : -1;

    const int rhs_a = rhs.a ? rhs.a->original_vertex_id : -1;
    const int rhs_b = rhs.b ? rhs.b->original_vertex_id : -1;
    const int rhs_c = rhs.c ? rhs.c->original_vertex_id : -1;
    const int rhs_d = rhs.d ? rhs.d->original_vertex_id : -1;

    // Smaller vertex IDs have higher priority
    if (lhs_b != rhs_b) return lhs_b > rhs_b;
    if (lhs_c != rhs_c) return lhs_c > rhs_c;
    if (lhs_a != rhs_a) return lhs_a > rhs_a;
    if (lhs_d != rhs_d) return lhs_d > rhs_d;
    return lhs.a > rhs.a;
}

bool CanFormCandidate(const Vertex* a, const Vertex* b, const Vertex* c, const Vertex* d) {
    if (a == nullptr || b == nullptr || c == nullptr || d == nullptr) {
        return false;
    }
    if (!a->active || !b->active || !c->active || !d->active) {
        return false;
    }
    if (!IsSameRing(a, b, c, d)) {
        return false;
    }

    std::set<const Vertex*> distinct{a, b, c, d};
    return distinct.size() == 4;
}

std::optional<CollapseCandidate> BuildCandidate(
    Vertex* a,
    Vertex* b,
    Vertex* c,
    Vertex* d
) {
    if (!CanFormCandidate(a, b, c, d)) {
        return std::nullopt;
    }

    CollapseCandidate candidate;
    candidate.a = a;
    candidate.b = b;
    candidate.c = c;
    candidate.d = d;
    
    auto result = ComputeBestAPSCReplacement(*a, *b, *c, *d);
    if (!result.has_value()) {
        return std::nullopt;
    }
    
    candidate.collapse = *result;
    candidate.a_version = a->version;
    candidate.b_version = b->version;
    candidate.c_version = c->version;
    candidate.d_version = d->version;
    return candidate;
}

bool IsCandidateStillValid(const CollapseCandidate& candidate) {
    if (!CanFormCandidate(candidate.a, candidate.b, candidate.c, candidate.d)) {
        return false;
    }
    if (candidate.a->next != candidate.b) return false;
    if (candidate.b->next != candidate.c) return false;
    if (candidate.c->next != candidate.d) return false;
    
    // Add version checks
    if (candidate.a->version != candidate.a_version) return false;
    if (candidate.b->version != candidate.b_version) return false;
    if (candidate.c->version != candidate.c_version) return false;
    if (candidate.d->version != candidate.d_version) return false;
    
    return true;
}

void PushCandidateIfPossible(
    CollapseQueue& queue,
    Vertex* a,
    Vertex* b,
    Vertex* c,
    Vertex* d
) {
    const std::optional<CollapseCandidate> candidate = BuildCandidate(a, b, c, d);
    if (candidate.has_value()) {
        queue.push(*candidate);
    }
}

void QueueLocalCandidatesAround(CollapseQueue& queue, Vertex* center) {
    if (center == nullptr || !center->active) {
        return;
    }

    Vertex* start = center;
    for (int i = 0; i < 3; ++i) {
        if (!start->prev) {
            return;
        }
        start = start->prev;
    }

    for (int i = 0; i < 4; ++i) {
        Vertex* const a = start;
        Vertex* const b = a ? a->next : nullptr;
        Vertex* const c = b ? b->next : nullptr;
        Vertex* const d = c ? c->next : nullptr;
        PushCandidateIfPossible(queue, a, b, c, d);
        start = start->next;
    }
}

CollapseQueue BuildInitialCollapseQueue(const std::map<int, Vertex*>& ring_heads) {
    CollapseQueue queue;

    for (const auto& [ring_id, head] : ring_heads) {
        (void)ring_id;
        if (head == nullptr || !head->active) {
            continue;
        }

        Vertex* current = head;
        do {
            Vertex* const b = current->next;
            Vertex* const c = b ? b->next : nullptr;
            Vertex* const d = c ? c->next : nullptr;
            PushCandidateIfPossible(queue, current, b, c, d);
            current = current->next;
        } while (current != nullptr && current != head);
    }

    return queue;
}

std::optional<CollapseCandidate> PopBestValidCandidate(CollapseQueue& queue) {
    while (!queue.empty()) {
        const CollapseCandidate candidate = queue.top();
        queue.pop();

        if (IsCandidateStillValid(candidate)) {
            return candidate;
        }
    }

    return std::nullopt;
}
