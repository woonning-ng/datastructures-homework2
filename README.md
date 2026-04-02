# CSD2183 Project 2

Build:

```sh
make
```

The current repository layout assumes `make` and the existing `simplify` executable are run through a bash-compatible environment. On Windows, use WSL/bash when building or running the program.

Run:

```sh
./simplify <input_file.csv> <target_vertices>
```

Example:

```sh
./simplify test_cases/input_rectangle_with_two_holes_scaled.csv 11
```

Test workflow:

The automated test workflow uses `test_cases/test_manifest.tsv` as the source of truth. Each row provides:

- input CSV
- target vertex count
- expected output file

Generated outputs are written to `generated_outputs/` using the same output filenames listed in `test_cases/test_manifest.tsv`.

Generate outputs for all documented tests:

```sh
make generate
```

The generator applies a per-test timeout of 30 seconds by default so a hanging case does not block the full suite. You can override it with `make generate TIMEOUT_SECONDS=<n>`.

Compare generated outputs against the expected files in `test_cases`:

```sh
make compare
```

Run the full workflow in one command:

```sh
make test
```

Makefile shortcuts:

```sh
make generate
make compare
make test
```

Comparison is exact byte-for-byte output matching. Failed comparisons print the expected path, generated path, and a compact unified diff. The compare step exits nonzero if any test fails.
