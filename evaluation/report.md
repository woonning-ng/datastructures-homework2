# Experimental Evaluation Report

## Scope

This evaluation extends the provided xSITe-style correctness tests with a custom dataset suite designed to stress distinct geometric and topological behaviors of the simplifier. The generated families are documented in [`datasets/README.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/datasets/README.md), and the raw/summary measurements are stored in [`results/`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results).

## Dataset families

| Family | Stress target | Why it is challenging |
|---|---|---|
| `convex_large` | Pure input-size scaling | High vertex count with minimal topology interference isolates queue maintenance and repeated local validity checks. |
| `wavy_boundary` | Irregular boundary noise | Many plausible low-displacement collapses appear, so the greedy choice still needs repeated geometry validation. |
| `near_collinear` | Near-degenerate local geometry | Very small local triangle areas stress floating-point behavior and make area-preserving replacement points sensitive. |
| `many_holes` | Ring-count/topology pressure | Dense interior rings sharply restrict valid collapses and often stop simplification before the requested target. |
| `narrow_gap` | Small clearances | Collapses that are locally attractive can become globally invalid because neighboring rings are close. |
| `scaled_irregular` | Coordinate magnitude sensitivity | The same topology is evaluated at two coordinate scales to check whether absolute coordinate size affects timing or memory. |

## Measurement methodology

- Benchmark driver: [`run_benchmarks.ps1`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/run_benchmarks.ps1)
- Preferred Unix-like method: `/usr/bin/time -v`
- Windows fallback used for the checked-in results: `Stopwatch` for elapsed time and live-sampled working set for peak memory
- Repetitions: 2 runs per dataset/target, averaged in [`benchmark_summary.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/benchmark_summary.csv) and [`displacement_summary.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/displacement_summary.csv)

## Performance results

The timing data grows meaningfully with input size for the single-ring scaling families. Representative examples from [`benchmark_table.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/benchmark_table.md):

- `convex_large`: 64 vertices -> 59.617 ms, 256 vertices -> 191.244 ms, 512 vertices -> 968.178 ms
- `wavy_boundary`: 64 vertices -> 42.293 ms, 256 vertices -> 197.068 ms, 512 vertices -> 991.095 ms
- `near_collinear`: 80 vertices -> 39.310 ms, 320 vertices -> 306.270 ms, 640 vertices -> 1777.031 ms

The fitted runtime models from [`model_fits.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/model_fits.md) are:

- power law: `0.332878 * n^1.187882`, RMSE `282.525`, `R^2 = 0.606711`
- `n log n`: `0.297806 * n log n`, RMSE `187.595`, `R^2 = 0.826604`

In this benchmark set, the `n log n` fit explains the runtime trend better than the power-law fit. That is consistent with the implementation structure: candidate selection is priority-queue based, but each accepted collapse still triggers enough local/global geometry work that the observed exponent is somewhat above linear.

## Memory results

Peak working-set usage stays in a narrow band:

- minimum observed: about `3.662 MiB`
- maximum observed: about `4.369 MiB`

The best power-law fit is close to flat:

- `4033837.098406 * n^0.011343`

That near-zero exponent matches the data better than the `c * n log n` alternative, which performs poorly. The main reason is that these experiments vary input size moderately while the executable’s fixed process overhead dominates peak working set on this platform.

## Areal displacement vs target vertex count

The target sweeps show the expected monotonic behavior for datasets that can continue simplifying:

- `convex_large_256`: displacement rises from `0.9610214` at target `230` to `29.04952` at target `64`
- `wavy_boundary_256`: displacement rises from `1.034131` at target `230` to `173.6105` at target `64`
- `near_collinear_320`: displacement rises from `0.05139599` at target `288` to `8.239717` at target `80`

This confirms the greedy method’s expected tradeoff: stronger simplification forces the algorithm to accept collapses with larger cumulative areal displacement.

The `many_holes_4x4` sweep is the clearest topology-limited anomaly:

- target `61` is reachable with displacement `1456.56`
- requests for targets `51` and below stop early at `52` output vertices with the same displacement `3329.28`

That plateau indicates the simplifier runs out of valid collapses before reaching the nominal target because preserving all 16 holes leaves too little geometric freedom.

## Geometry-specific observations

- `near_collinear` grows fastest in runtime among the one-ring families, which is plausible because nearly degenerate configurations increase the number of collapses that must be validated carefully.
- `narrow_gap` becomes noticeably slower than similarly sized simple rings, especially at `406` vertices, because close ring-to-ring clearances make global validity checks reject more locally good candidates.
- `scaled_irregular_small` and `scaled_irregular_large` have nearly identical time and memory (`108.873 ms` vs `101.512 ms`, `3.965 MiB` vs `3.990 MiB`), which suggests coordinate magnitude alone does not dominate runtime in this implementation.
- The large-scale irregular dataset has much larger absolute areal displacement than the small-scale version because the same geometric changes are measured in a much larger coordinate system.

## Limitations

- The checked-in result files were generated on Windows using the benchmark runner’s fallback memory measurement. On Unix-like systems, the same script will prefer `/usr/bin/time -v`, which is the recommended method for the final assignment run.
- Peak memory is relatively flat in this environment, so asymptotic interpretation is weaker for memory than for runtime.
- The runtime fit pools multiple geometry families together. That is useful for a repository-wide overview, but family-specific fits could be added later if a finer asymptotic study is required.

## Reproduction

```powershell
pwsh -File evaluation/run_all.ps1
```

The main outputs are:

- [`datasets/manifest.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/datasets/manifest.csv)
- [`results/benchmark_summary.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/benchmark_summary.csv)
- [`results/displacement_summary.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/displacement_summary.csv)
- [`results/model_fits.csv`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/model_fits.csv)
- [`plots/runtime_vs_input_size.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/runtime_vs_input_size.svg)
- [`plots/memory_vs_input_size.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/memory_vs_input_size.svg)
- [`plots/areal_displacement_vs_target.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/areal_displacement_vs_target.svg)
