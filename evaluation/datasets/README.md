# Custom Evaluation Datasets

All datasets in this folder use the required 'ring_id,vertex_id,x,y' CSV format.

| Dataset family | Sizes | Stress property | Why it is meaningful |
|---|---:|---|---|
| convex_large | 64, 128, 256, 512 | High vertex count on a simple convex exterior ring. | Separates algorithmic overhead from topology complexity by forcing many candidate updates on a geometry with no holes. |
| many_holes | 20, 40, 68, 104 | Many interior rings and dense ring-to-ring interaction checks. | Topology preservation dominates because every collapse must avoid both self-intersections and hole collisions. |
| narrow_gap | 118, 214, 406 | Small clearances between exterior ring and nearby holes. | A valid local area-preserving point can become globally invalid because tiny movements may intersect a neighboring ring. |
| near_collinear | 80, 160, 320, 640 | Nearly collinear chains and skinny geometry. | Stresses floating-point robustness, small triangle areas, and collapse validity near degenerate configurations. |
| scaled_irregular | 197, 197 | Coordinate scale sensitivity with identical topology and vertex count. | Tests whether large coordinate magnitudes affect timing, memory, or numerical stability even when combinatorial structure is unchanged. |
| wavy_boundary | 64, 128, 256, 512 | Irregular noisy outer boundary. | Creates many low-displacement local collapses with nontrivial geometry, increasing the need for careful validity checks. |

Generated manifest: [manifest.csv](manifest.csv).
