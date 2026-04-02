#!/usr/bin/env bash

set -euo pipefail

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/test_workflow.sh"

DIFF_LINES="${1:-40}"

compare_outputs "$DIFF_LINES"
