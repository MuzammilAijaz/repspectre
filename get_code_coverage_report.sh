#!/usr/bin/env bash
set -e

BUILD_DIR="build"
COV_DIR="coverage"
REPORT="$COV_DIR/index.html"

mkdir -p "$COV_DIR"

echo "Running tests..."
# ctest --test-dir "$BUILD_DIR" -L unit --output-on-failure
make test

echo "Generating coverage..."
gcovr -r . "$BUILD_DIR" \
	--html --html-details \
	-o "$REPORT"

echo "Opening in Zen Browser..."
zen-browser "$REPORT"
