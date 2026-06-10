#!/usr/bin/env bash

# -----------------------------------------------------------------------------
# Launches a tmux-based ESP32-S3 debugging environment for a HIL test.
#
# Creates a tmux window/session containing:
#   - OpenOCD server
#   - GDB connected to the test ELF
#   - debug_esp.sh helper script
#
# Usage:
#   ./start_debug.sh <test_name>
#
# The script locates the test ELF in the build directory, starts the required
# debug tools, arranges them in a tiled tmux layout, and attaches/switches to
# the created tmux window.
# -----------------------------------------------------------------------------

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <test_name>"
    exit 1
fi

TEST_NAME="$1"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Project root (adjust if needed)
PROJECT_ROOT="$(realpath "$SCRIPT_DIR/../../..")"

# find ELF
ELF=$(find "${PROJECT_ROOT}/build/hil_work_${TEST_NAME}" \
	-name "${TEST_NAME}.ino.elf" | head -n1)

if [[ -z "$ELF" ]]; then
    echo "ELF not found for test: $TEST_NAME"
    exit 1
fi

# Determine if we are inside tmux and create target accordingly
if [[ -n "$TMUX" ]]; then
    # Inside tmux: create a new detached window (-d) in the current session
    # We ask tmux to print (-P) the new session/window ID to target it precisely
    TARGET=$(tmux new-window -d -n "$TEST_NAME" -P -F "#{session_id}:#{window_id}")
else
    # Outside tmux: create a new session (auto-named by tmux) with a named window
    TARGET=$(tmux new-session -d -n "$TEST_NAME" -P -F "#{session_id}:#{window_id}")
fi

# pane 0: openocd
tmux send-keys -t "${TARGET}.0" \
    'openocd-esp32openocd -f board/esp32s3-builtin.cfg' C-m

sleep 1

# pane 1: gdb
tmux split-window -h -t "${TARGET}.0"

tmux send-keys -t "${TARGET}.1" \
    "xtensa-esp32s3-elf-gdb $ELF \
    -ex 'target extended-remote localhost:3333' \
    -ex 'monitor reset halt'" C-m

# pane 2: helper
tmux split-window -v -t "${TARGET}.1"

tmux send-keys -t "${TARGET}.2" \
    "${SCRIPT_DIR}/debug_esp.sh ${TEST_NAME}" C-m

# organize layout and select the helper script pane
tmux select-layout -t "${TARGET}" tiled
tmux select-pane -t "${TARGET}.2"

# Attach or switch properly depending on context
if [[ -n "$TMUX" ]]; then
    # We are already attached, just switch the current client's view to the new window
    tmux select-window -t "${TARGET}"
else
    # Outside of tmux, attach to the newly created session
    tmux attach-session -t "${TARGET}"
fi
