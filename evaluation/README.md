# Experimental Evaluation

This folder contains a reproducible evaluation pipeline for the polygon simplifier:

1. 'generate_datasets.ps1'
2. 'run_benchmarks.ps1'
3. 'fit_models.ps1'
4. 'plot_results.ps1'
5. 'run_all.ps1'

## Measurement methodology

- On Unix-like systems, 'run_benchmarks.ps1' prefers '/usr/bin/time -v'.
- On Windows, it falls back to 'Stopwatch' plus the child process 'PeakWorkingSet64'.

## Outputs

- 'datasets/manifest.csv'
- 'results/benchmark_raw.csv'
- 'results/benchmark_summary.csv'
- 'results/displacement_raw.csv'
- 'results/displacement_summary.csv'
- 'results/model_fits.csv'
- 'results/benchmark_table.md'
- 'results/model_fits.md'
- 'plots/runtime_vs_input_size.svg'
- 'plots/memory_vs_input_size.svg'
- 'plots/areal_displacement_vs_target.svg'
