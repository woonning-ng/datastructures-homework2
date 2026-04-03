#!/usr/bin/env bash

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_CASES_DIR="$REPO_ROOT/test_cases"
GENERATED_OUTPUTS_DIR="$REPO_ROOT/generated_outputs"
TEST_CASES_MANIFEST="$TEST_CASES_DIR/test_manifest.tsv"
DEFAULT_TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-30}"

discover_test_cases() {
    tail -n +2 "$TEST_CASES_MANIFEST"
}

ensure_executable() {
    if [[ -x "$REPO_ROOT/simplify" ]]; then
        printf '%s\n' "$REPO_ROOT/simplify"
        return 0
    fi
    if [[ -x "$REPO_ROOT/simplify.exe" ]]; then
        printf '%s\n' "$REPO_ROOT/simplify.exe"
        return 0
    fi
    printf 'Could not find built executable "simplify" or "simplify.exe".\n' >&2
    return 1
}

build_if_needed() {
    if [[ "${NO_BUILD:-0}" == "1" ]]; then
        return 0
    fi
    make -C "$REPO_ROOT"
}

normalize_generated_output() {
    local generated_path normalized_path
    generated_path="$1"
    normalized_path="$(mktemp)"
    awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "$generated_path" > "$normalized_path"
    mv "$normalized_path" "$generated_path"
}

generate_outputs() {
    local executable timeout_seconds failures=0 total=0
    executable="$(ensure_executable)"
    timeout_seconds="${1:-$DEFAULT_TIMEOUT_SECONDS}"

    mkdir -p "$GENERATED_OUTPUTS_DIR"

    while IFS=$'\t' read -r input_file target_vertices output_file; do
        [[ -n "$input_file" ]] || continue
        total=$((total + 1))

        local input_path expected_path generated_path stderr_path
        input_path="$TEST_CASES_DIR/$input_file"
        expected_path="$TEST_CASES_DIR/$output_file"
        generated_path="$GENERATED_OUTPUTS_DIR/$output_file"
        stderr_path="$(mktemp)"

        mkdir -p "$(dirname "$generated_path")"
        : > "$generated_path"

        if timeout "$timeout_seconds" "$executable" "$input_path" "$target_vertices" >"$generated_path" 2>"$stderr_path"; then
            normalize_generated_output "$generated_path"
            printf 'GENERATED %s (target %s) -> generated_outputs/%s\n' "${input_file%.csv}" "$target_vertices" "$output_file"
        else
            status=$?
            failures=$((failures + 1))
            if [[ "$status" -eq 124 ]]; then
                printf 'FAIL %s: timed out after %s second(s)\n' "${input_file%.csv}" "$timeout_seconds"
            else
                printf 'FAIL %s: program exited with code %s\n' "${input_file%.csv}" "$status"
            fi
            printf '  input: %s\n' "$input_path"
            printf '  expected: %s\n' "$expected_path"
            printf '  generated: %s\n' "$generated_path"
            if [[ -s "$stderr_path" ]]; then
                printf '  stderr:\n'
                sed 's/^/    /' "$stderr_path"
            fi
        fi

        rm -f "$stderr_path"
    done < <(discover_test_cases)

    printf '\n'
    if [[ "$failures" -gt 0 ]]; then
        printf 'Generation finished with %s failed run(s).\n' "$failures" >&2
        return 1
    fi

    printf 'Generated %s output file(s).\n' "$total"
}

compare_outputs() {
    local diff_lines passed=0 failed=0 total=0
    diff_lines="${1:-40}"

    while IFS=$'\t' read -r input_file target_vertices output_file; do
        [[ -n "$input_file" ]] || continue
        total=$((total + 1))
        local expected_path generated_path test_name
        test_name="${input_file%.csv}"
        expected_path="$TEST_CASES_DIR/$output_file"
        generated_path="$GENERATED_OUTPUTS_DIR/$output_file"

        if [[ ! -f "$generated_path" ]]; then
            failed=$((failed + 1))
            printf 'FAIL %s\n' "$test_name"
            printf '  expected: %s\n' "$expected_path"
            printf '  actual:   %s\n' "$generated_path"
            printf '  reason: generated output file is missing\n'
            continue
        fi

        if cmp -s "$expected_path" "$generated_path"; then
            passed=$((passed + 1))
            printf 'PASS %s\n' "$test_name"
            continue
        fi

        failed=$((failed + 1))
        printf 'FAIL %s\n' "$test_name"
        printf '  expected: %s\n' "$expected_path"
        printf '  actual:   %s\n' "$generated_path"
        printf '  diff:\n'
        diff -u "$expected_path" "$generated_path" | sed -n "1,${diff_lines}p" | sed 's/^/    /' || true
    done < <(discover_test_cases)

    printf '\nSummary: %s passed, %s failed, %s total\n' "$passed" "$failed" "$total"
    [[ "$failed" -eq 0 ]]
}
