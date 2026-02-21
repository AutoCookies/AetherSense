#!/usr/bin/env bash
set -euo pipefail
build_dir=${1:-build}
"$build_dir/apps/aethersense_cli" --config testdata/sample_config.json --output jsonl > /tmp/aethersense_soak.log
