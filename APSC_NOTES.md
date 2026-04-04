# APSC Local Core Notes

This repo now has a reusable local APSC helper in
[math.hpp](/C:/Users/varic/OneDrive/Desktop/SIT/CSD%202183%20data%20structure/grp%20ass%202/datastructures-homework2/math.hpp)
and
[math.cpp](/C:/Users/varic/OneDrive/Desktop/SIT/CSD%202183%20data%20structure/grp%20ass%202/datastructures-homework2/math.cpp).

## Public API

```cpp
std::optional<APSCCollapseResult> ComputeBestAPSCReplacement(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D
);
```

`APSCCollapseResult` contains:

- `point`: the chosen area-preserving Steiner point `E`
- `local_displacement`: the local areal-displacement cost
- `side`: whether `E` came from the `AB` side or the `CD` side

## What it computes

For four consecutive vertices `A, B, C, D`:

- it constructs the two standard area-preserving candidates for `E`
- one candidate lies on the line through `AB`
- one candidate lies on the line through `CD`
- it computes the local symmetric-difference cost for each candidate
- it returns the lower-cost candidate

## How teammates should use it

- Ring-list teammate: pass four consecutive active vertices directly into `ComputeBestAPSCReplacement(...)`.
- Priority-queue teammate: use `result->local_displacement` as the key.
- Topology teammate: validate the returned `A-E-D` replacement before committing the collapse.

## Lower-level helpers

If a teammate needs the side-specific pieces:

- `ComputeAreaPreservingPointE(...)`
- `ComputeLocalArealDisplacement(...)`

## Test-case note

The rectangle README target still says `7`, but the reference output keeps `11` vertices
(4 outer + 3 hole + 4 hole), so the reference file looks consistent with target `11`, not `7`.
