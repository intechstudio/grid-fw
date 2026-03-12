idf_component_register(

    LDFRAGMENTS
        "linker_fragment.lf"

    SRC_DIRS
        "common/src/c"
        "common/dep/littlefs"
        "common/dep/lua-5.4.3/src"
        "common/dep/tomlc99"

    INCLUDE_DIRS
        "."
        "common/src/c"
        "common/dep"
        "common/dep/littlefs"
        "common/dep/lua-5.4.3/src"
        "common/dep/proto"
        "common/build/lua"
)
