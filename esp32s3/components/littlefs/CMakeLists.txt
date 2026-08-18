# Local littlefs component — compiles upstream littlefs-project/littlefs
# from the git submodule at common/dep/littlefs (pinned to tag v2.7.1).

# Path to the submodule relative to this CMakeLists.txt
set(LFS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../common/dep/littlefs")

idf_component_register(
    SRCS
        "${LFS_ROOT}/lfs.c"
        "${LFS_ROOT}/lfs_util.c"
    INCLUDE_DIRS
        "${LFS_ROOT}"
)
