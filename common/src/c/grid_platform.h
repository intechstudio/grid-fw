#ifndef GRID_PLATFORM_H
#define GRID_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#define GRID_UI_CONFIG_PATH "config.toml"

extern void grid_platform_set_lfs(void* lfs);

// LittleFS has no locking in this build (LFS_THREADSAFE is never defined,
// and no lock/unlock callbacks are wired into struct lfs_config anywhere in
// this tree) - grid_platform_fopen/fread/fwrite/fclose/stat and friends
// below are NOT safe to call concurrently from more than one task. By
// convention, only the task that runs Lua (script/config persistence in
// grid_ui.c, and any image/file loading triggered from Lua, e.g. load_file
// in grid_lua_api_gui.c) may call these. Do not add a second caller on a
// different task without adding real locking first - the LCD render task
// briefly did exactly that (a since-fixed bug) before this comment existed.
extern void* grid_platform_fopen(const char* pathname, const char* mode);

extern int grid_platform_fclose(void* stream);

extern size_t grid_platform_fwrite(const void* ptr, size_t size, size_t nmemb, void* stream);

extern size_t grid_platform_fread(void* ptr, size_t size, size_t nmemb, void* stream);

extern long grid_platform_ftell(void* stream);

extern int grid_platform_fseek(void* stream, long offset, int whence);

extern void grid_platform_clearerr(void* stream);

extern int grid_platform_ferror(void* stream);

extern int grid_platform_getc(void* stream);

extern int grid_platform_ungetc(int c, void* stream);

extern int grid_platform_fflush(void* stream);

extern int grid_platform_remove(const char* pathname);

extern int grid_platform_rename(const char* oldpath, const char* newpath);

extern void* grid_platform_opendir(const char* name);

extern int grid_platform_closedir(void* dirp);

extern void* grid_platform_readdir(void* dirp);

extern const char* grid_platform_file_info_name(void* info);

extern uint8_t grid_platform_file_info_type(void* info);

extern uint32_t grid_platform_file_info_size(void* info);

extern void grid_platform_printf(char const* fmt, ...);

extern void grid_platform_printf_nonprint(const uint8_t* src, size_t size);

extern uint32_t grid_platform_get_id(uint32_t* return_array);

extern uint32_t grid_platform_get_hwcfg();

extern uint32_t grid_platform_get_hwcfg_bit(uint8_t n);

extern uint8_t grid_platform_get_random_8();

extern uint8_t grid_platform_get_reset_cause();

extern uint64_t grid_platform_rtc_get_micros();

extern uint64_t grid_platform_rtc_get_diff(uint64_t t1, uint64_t t2);

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_nvm_defrag();

extern void grid_platform_system_reset();

extern uint32_t grid_platform_get_frame_len(uint8_t dir);

extern void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir);

extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);

extern void* grid_platform_allocate_volatile(size_t size);

extern char* grid_platform_read_file_contents(const char* path);

extern int grid_platform_write_file_contents(const char* buf, const char* path);

extern void* grid_platform_dir_first(const char* path);

extern void grid_platform_delete_script_files_all();

extern void grid_platform_nvm_erase();

extern void grid_platform_nvm_format_and_mount();

extern const char* grid_platform_get_base_path();

extern int grid_platform_mkdir(const char* path);

extern int grid_platform_lsdir(const char* path);

extern int grid_platform_stat(const char* path, void** statbuf);

extern uint8_t grid_platform_get_nvm_state();

extern uint8_t grid_platform_erase_nvm_next();

extern void grid_platform_lcd_set_backlight(uint8_t backlight);

extern uint8_t grid_platform_get_adc_bit_depth();

extern void grid_platform_mux_init(uint8_t mux_positions_bm);

extern void grid_platform_mux_write(uint8_t index);

extern void grid_platform_id_to_hex(const uint8_t* id, uint8_t byte_count, char* out);

extern void grid_platform_byte_to_hex(uint8_t byte, char out[2]);

#endif /* GRID_PLATFORM_H */
