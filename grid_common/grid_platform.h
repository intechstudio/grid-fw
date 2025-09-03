#ifndef GRID_PLATFORM_H
#define GRID_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

extern void grid_platform_printf(char const* fmt, ...);

extern uint32_t grid_platform_get_id(uint32_t* return_array);

extern uint32_t grid_platform_get_hwcfg();

extern uint32_t grid_platform_get_hwcfg_bit(uint8_t n);

extern uint8_t grid_platform_get_random_8();

extern uint8_t grid_platform_get_reset_cause();

extern uint64_t grid_platform_rtc_get_micros();

extern uint64_t grid_platform_rtc_get_elapsed_time(uint64_t told);

extern void grid_platform_nvm_defrag();

extern void grid_platform_system_reset();

extern uint32_t grid_platform_get_frame_len(uint8_t dir);

extern void grid_platform_send_frame(void* swsr, uint32_t size, uint8_t dir);

extern uint8_t grid_platform_reset_grid_transmitter(uint8_t direction);

extern int32_t grid_platform_usb_serial_ready();

extern int32_t grid_platform_usb_serial_write(char* buffer, uint32_t length);

extern void* grid_platform_allocate_volatile(size_t size);

union grid_ui_file_handle;

extern int grid_platform_find_next_actionstring_file_on_page(uint8_t page, int* last_element, int* last_event, union grid_ui_file_handle* file_handle);

extern int grid_platform_find_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, union grid_ui_file_handle* file_handle);

extern void grid_platform_delete_actionstring_file(union grid_ui_file_handle* file_handle);

extern uint16_t grid_platform_get_actionstring_file_size(union grid_ui_file_handle* file_handle);

extern uint32_t grid_platform_read_actionstring_file_contents(union grid_ui_file_handle* file_handle, char* targetstring, uint16_t size);

extern void grid_platform_write_actionstring_file(uint8_t page, uint8_t element, uint8_t event_type, char* buffer, uint16_t length);

extern void grid_platform_clear_all_actionstring_files_from_page(uint8_t page);

extern void grid_platform_delete_actionstring_files_all();

extern void grid_platform_nvm_erase();

extern const char* grid_platform_get_base_path();

extern int grid_platform_make_directory(const char* path);

extern int grid_platform_list_directory(const char* path);

extern int grid_platform_find_file(const char* path, union grid_ui_file_handle* file_handle);

extern uint16_t grid_platform_get_file_size(union grid_ui_file_handle* file_handle);

extern int grid_platform_read_file(union grid_ui_file_handle* file_handle, uint8_t* buffer, uint16_t size);

extern int grid_platform_write_file(char* path, uint8_t* buffer, uint16_t size);

extern int grid_platform_delete_file(union grid_ui_file_handle* file_handle);

extern int grid_platform_remove_path(const char* path);

extern int grid_platform_remove_dir(const char* path);

extern uint8_t grid_platform_get_nvm_state();

extern uint8_t grid_platform_erase_nvm_next();

extern void grid_platform_lcd_set_backlight(uint8_t backlight);

#endif /* GRID_PLATFORM_H */
