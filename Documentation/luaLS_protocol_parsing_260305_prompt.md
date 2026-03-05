---
kkerti, 2026-03-05
---
## Plan: Hybrid `_usage` + LuaCATS Documentation Pipeline

**TL;DR**: `_usage` macros in grid-fw headers (firmware engineers). LuaCATS stubs in grid-protocol (anyone). A coverage script checks `_usage` completeness. Protocol converter emits structured `"luadocs"` in the bot JSON. A scaffold script generates **only new** stub entries — existing functions in the LuaCATS files are never touched.

**Steps**

1. **`_usage` macros in grid-fw** — add missing `_usage` for `segment_calculate` and `map_saturate` in `lua_source_collection.h`, and `button_value` in `grid_protocol.h`. Existing format: `"human_name(type param, ...) Description."`.

2. **Coverage diagnostic** `.github/workflows/usage_coverage.sh` — scans the 8 input headers. For every `_human` macro, checks for matching `_usage`. Reports percentage and missing list. Same grep style as `documentation_validator.sh`. Added as a CI step in `protocol_converter.yml`.

3. **Extend `protocol_converter.py`** — `create_lua_docs()` groups `GRID_LUA_FNC_` macros by prefix, merges `_short`, `_human`, `_usage` into structured entries. Writes as `"luadocs"` key in `grid_protocol_bot.json`. Backward-compatible.

4. **LuaCATS stubs in grid-protocol** — two files in `annotations/`:
   - `globals.lua` — `---@meta` stubs for global functions (`midi_send`, `segment_calculate`, etc.)
   - `elements.lua` — `---@class` stubs for element types (`ButtonElement`, `PotmeterElement`, etc.) with methods

   POC covers: `midi_send`, `segment_calculate`, `map_saturate` (globals) + `ButtonElement.button_value` (element).

5. **Scaffold script** `scripts/scaffold_annotations.py` in grid-protocol — **merge-safe by design**:
   - Reads `grid_protocol_bot.json` `"luadocs"` for the full function list
   - Reads existing `annotations/globals.lua` and `annotations/elements.lua`
   - Parses existing stubs by extracting every `function <name>` declaration already present
   - For each function in `"luadocs"`: if the function name **already exists** in the annotation file, **skip it entirely** — no overwrite, no merge, no modification
   - For each function that is **new** (present in JSON but not in the stub file), append a skeleton stub at the end of the appropriate file, marked with a `-- [scaffold]` comment so reviewers can spot auto-generated entries
   - Print a summary: `Added: 3 new stubs. Skipped: 47 existing. See [scaffold] markers.`

   This ensures: a firmware engineer adds a `_usage` → protocol converter picks it up → scaffold appends a bare skeleton → a contributor enriches it with `---@param`, `---@return`, ranges, overloads → scaffold never overwrites that work.

6. **Grid-editor Monaco integration** — for POC, parse `"luadocs"` from JSON at runtime, register `CompletionItemProvider` + `HoverProvider` + `SignatureHelpProvider`. LuaCATS stubs are ready for future luaLS WASM integration.

**Verification**

- Run `usage_coverage.sh` locally — confirm coverage report
- Run `protocol_converter.py` — verify `"luadocs"` key in JSON
- Run `scaffold_annotations.py` once → generates stubs. Run again → `Added: 0 new stubs. Skipped: N existing.` (idempotent)
- Manually edit a stub, re-run scaffold → edited stub is unchanged

**Decisions**

- **Append-only scaffold**: Never modifies or deletes existing function stubs. Only adds new ones. Function name match is the identity key.
- **`-- [scaffold]` marker**: Distinguishes auto-generated skeletons from hand-enriched annotations. Optional to remove after enrichment.
- **grid-fw owns `_usage`**, **grid-protocol owns LuaCATS stubs**: Clean ownership boundary. Scaffold bridges the two without coupling.
