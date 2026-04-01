#include "Simplifier.hpp"

#include <set>

namespace {

std::size_t g_next_candidate_sequence = 0;

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
    if (lhs.collapse.local_displacement != rhs.collapse.local_displacement) {
        return lhs.collapse.local_displacement > rhs.collapse.local_displacement;
    }
    if (lhs.sequence_number != rhs.sequence_number) {
        return lhs.sequence_number > rhs.sequence_number;
    }
    return static_cast<int>(lhs.collapse.side) > static_cast<int>(rhs.collapse.side);
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

    const std::optional<APSCCollapseResult> collapse = ComputeBestAPSCReplacement(*a, *b, *c, *d);
    if (!collapse.has_value()) {
        return std::nullopt;
    }

    CollapseCandidate candidate;
    candidate.a = a;
    candidate.b = b;
    candidate.c = c;
    candidate.d = d;
    candidate.collapse = *collapse;
    candidate.a_version = a->version;
    candidate.b_version = b->version;
    candidate.c_version = c->version;
    candidate.d_version = d->version;
    candidate.sequence_number = g_next_candidate_sequence++;
    return candidate;
}

bool IsCandidateStillValid(const CollapseCandidate& candidate) {
    if (!CanFormCandidate(candidate.a, candidate.b, candidate.c, candidate.d)) {
        return false;
    }

    if (candidate.a->version != candidate.a_version ||
        candidate.b->version != candidate.b_version ||
        candidate.c->version != candidate.c_version ||
        candidate.d->version != candidate.d_version) {
        return false;
    }

    return candidate.a->next == candidate.b &&
           candidate.b->prev == candidate.a &&
           candidate.b->next == candidate.c &&
           candidate.c->prev == candidate.b &&
           candidate.c->next == candidate.d &&
           candidate.d->prev == candidate.c;
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

    Vertex* const p2 = center->prev ? center->prev->prev : nullptr;
    Vertex* const p1 = center->prev;
    Vertex* const n1 = center->next;
    Vertex* const n2 = n1 ? n1->next : nullptr;
    Vertex* const n3 = n2 ? n2->next : nullptr;

    PushCandidateIfPossible(queue, p2, p1, center, n1);
    PushCandidateIfPossible(queue, p1, center, n1, n2);
    PushCandidateIfPossible(queue, center, n1, n2, n3);
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
