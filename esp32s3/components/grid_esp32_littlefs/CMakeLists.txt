idf_component_register(
    SRCS
        "grid_esp32_littlefs.c"
				"grid_esp32_littlefs_api.c"
    INCLUDE_DIRS
        "."
		REQUIRES
				"esp_partition" "grid_common"
)
