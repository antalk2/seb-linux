#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

make -j"$(nproc)" -C "${BUILD_DIR}"
echo "Build output: ${BUILD_DIR}/bin/safe-exam-browser"
