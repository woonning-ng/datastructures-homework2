#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/test_workflow.sh"

TIMEOUT_SECONDS="${1:-$DEFAULT_TIMEOUT_SECONDS}"
DIFF_LINES="${2:-40}"
NO_BUILD="${NO_BUILD:-0}"

build_if_needed
generate_outputs "$TIMEOUT_SECONDS"
compare_outputs "$DIFF_LINES"
