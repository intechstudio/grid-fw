idf_component_register(

    LDFRAGMENTS
        "linker_fragment.lf"

    SRC_DIRS
        "common/src/c"
        "common/dep/lua-5.5.0/src"
        "common/dep/tomlc99"

    INCLUDE_DIRS
        "."
        "common/src/c"
        "common/dep"
        "common/dep/lua-5.5.0/src"
        "common/dep/proto"
        "common/build/lua"

    REQUIRES
        "tinyusb"
        "littlefs"
)
