# CSD2183 Project 2

## Build

Unix-like:

```sh
make
```

Windows PowerShell:

```powershell
pwsh -File evaluation/build_simplify.ps1
```

## Run

```sh
./simplify <input_file.csv> <target_vertices>
```

Example:

```sh
./simplify test_cases/input_rectangle_with_two_holes_scaled.csv 11
```

## Experimental Evaluation

The repository now includes a full evaluation pipeline under [`evaluation/`](/C:/Users/trish/Documents/datastructures-homework2/evaluation).

Generate custom datasets:

```powershell
pwsh -File evaluation/generate_datasets.ps1
```

Run benchmarks:

```powershell
pwsh -File evaluation/run_benchmarks.ps1
```

Fit scaling models:

```powershell
pwsh -File evaluation/fit_models.ps1
```

Render plots:

```powershell
pwsh -File evaluation/plot_results.ps1
```

Run everything end-to-end:

```powershell
pwsh -File evaluation/run_all.ps1
```

Generated artifacts:

- datasets: [`evaluation/datasets/README.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/datasets/README.md)
- benchmark tables: [`evaluation/results/benchmark_table.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/benchmark_table.md)
- model fits: [`evaluation/results/model_fits.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/results/model_fits.md)
- report: [`evaluation/report.md`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/report.md)
- plots:
  - [`evaluation/plots/runtime_vs_input_size.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/runtime_vs_input_size.svg)
  - [`evaluation/plots/memory_vs_input_size.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/memory_vs_input_size.svg)
  - [`evaluation/plots/areal_displacement_vs_target.svg`](/C:/Users/trish/Documents/datastructures-homework2/evaluation/plots/areal_displacement_vs_target.svg)
