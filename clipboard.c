// disconnect

grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_RED, 50);
grid_alert_all_set_frequency(&grid_led_state, -2);
grid_alert_all_set_phase(&grid_led_state, 100);

// connect

grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_GREEN, 50);
grid_alert_all_set_frequency(&grid_led_state, -2);
grid_alert_all_set_phase(&grid_led_state, 100);