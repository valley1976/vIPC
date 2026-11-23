#!/usr/bin/env bash
# Copyright (c) 2026 shangu.
# Developed with DSH (DeepSeek Harness).

set -euo pipefail

examples_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$examples_dir/minimal/build.sh"
"$examples_dir/valley_drive/build.sh"
