#!/bin/bash
# usage_coverage.sh — checks that every GRID_LUA_FNC_*_human macro
# has a matching GRID_LUA_FNC_*_usage macro in the input headers.
# Same grep style as documentation_validator.sh.

set -euo pipefail

INPUT_FILES=(
  "./grid_common/grid_protocol.h"
  "./grid_common/grid_ui_system.h"
  "./grid_common/grid_ui_button.h"
  "./grid_common/grid_ui_encoder.h"
  "./grid_common/grid_ui_endless.h"
  "./grid_common/grid_ui_potmeter.h"
  "./grid_common/grid_ui_lcd.h"
  "./grid_common/lua_src/lua_source_collection.h"
)

pass=0
fail=0
missing=()

# Collect all unique GRID_LUA_FNC_ prefixes that have a _human macro
prefixes=$(grep -h 'GRID_LUA_FNC_.*_human' "${INPUT_FILES[@]}" \
  | sed -n 's/.*\(GRID_LUA_FNC_[A-Za-z0-9_]*\)_human.*/\1/p' \
  | sort -u)

for prefix in $prefixes; do
  # Check if a matching _usage macro exists in any input file
  if grep -q "${prefix}_usage" "${INPUT_FILES[@]}" 2>/dev/null; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1))
    # Extract the human-readable name for the report
    human_name=$(grep -h "${prefix}_human" "${INPUT_FILES[@]}" \
      | head -1 \
      | sed -n 's/.*"\([^"]*\)".*/\1/p')
    missing+=("${prefix} (${human_name})")
  fi
done

total=$((pass + fail))
if [ "$total" -eq 0 ]; then
  echo "No GRID_LUA_FNC_*_human macros found."
  exit 1
fi

coverage=$((100 * pass / total))
echo "Usage coverage: ${coverage}%  (${pass} have _usage, ${fail} missing, ${total} total)"

if [ "${#missing[@]}" -gt 0 ]; then
  echo ""
  echo "Missing _usage macros:"
  for entry in "${missing[@]}"; do
    echo "  - ${entry}"
  done
fi

# Exit with failure if coverage is not 100% (optional — remove for advisory-only mode)
# if [ "$fail" -gt 0 ]; then
#   exit 1
# fi
