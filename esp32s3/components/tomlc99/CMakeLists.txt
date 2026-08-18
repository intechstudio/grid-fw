# Local tomlc99 component — compiles upstream cktan/tomlc99 from the
# git submodule at common/dep/tomlc99 (pinned to commit 29076dfd, no
# tagged releases exist upstream).

# Path to the submodule relative to this CMakeLists.txt
set(TOML_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../common/dep/tomlc99")

idf_component_register(
    SRCS
        "${TOML_ROOT}/toml.c"
    INCLUDE_DIRS
        "${TOML_ROOT}"
)

# toml.c calls isdigit() on plain `char` values (e.g. isdigit(*p)), which
# trips -Wchar-subscripts since char may be signed — suppress the warning.
set_source_files_properties(
    "${TOML_ROOT}/toml.c"
    PROPERTIES COMPILE_FLAGS "-Wno-char-subscripts"
)
