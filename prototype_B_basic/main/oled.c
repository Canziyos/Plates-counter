// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_timer.h"
// #include "esp_lcd_panel_io.h"
// #include "esp_lcd_panel_ops.h"
// #include "driver/i2c.h"
// #include "esp_err.h"
// #include "esp_log.h"
// #include "lvgl.h"
// #include "esp_lvgl_port.h"
// #include "oled.h"

// // If CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107 is defined, use SH1107; otherwise, default to vendor-specific or SSD1306
// #if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
// #include "esp_lcd_sh1107.h"
// #else
// #include "esp_lcd_panel_vendor.h" // This might need to be changed to your specific driver, like "esp_lcd_ssd1306.h"
// #endif

// static const char *TAG = "example";

// #define I2C_HOST  0

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
// #define EXAMPLE_PIN_NUM_SDA           4
// #define EXAMPLE_PIN_NUM_SCL           15
// #define EXAMPLE_PIN_NUM_RST           16
// #define EXAMPLE_I2C_HW_ADDR           0x3C

// // The pixel number in horizontal and vertical
// #if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
// #define EXAMPLE_LCD_H_RES              128
// #define EXAMPLE_LCD_V_RES              64
// #elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
// #define EXAMPLE_LCD_H_RES              64
// #define EXAMPLE_LCD_V_RES              128
// #endif
// // Bit number used to represent command and parameter
// #define EXAMPLE_LCD_CMD_BITS           8
// #define EXAMPLE_LCD_PARAM_BITS         8

// lv_disp_t *disp = NULL;

// /* The LVGL port component calls esp_lcd_panel_draw_bitmap API for send data to the screen. There must be called
// lvgl_port_flush_ready(disp) after each transaction to display. The best way is to use on_color_trans_done
// callback from esp_lcd IO config structure. In IDF 5.1 and higher, it is solved inside LVGL port component. */
// static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
//     lv_disp_t *disp_local = (lv_disp_t *)user_ctx;
//     lvgl_port_flush_ready(disp_local);
//     return false;
// }


// void initialize_oled(void)
// {
//     ESP_LOGI(TAG, "Initialize I2C bus");
//     i2c_config_t i2c_conf = {
//         .mode = I2C_MODE_MASTER,
//         .sda_io_num = EXAMPLE_PIN_NUM_SDA,
//         .scl_io_num = EXAMPLE_PIN_NUM_SCL,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
//     };
//     ESP_ERROR_CHECK(i2c_param_config(I2C_HOST, &i2c_conf));
//     ESP_ERROR_CHECK(i2c_driver_install(I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));

//     ESP_LOGI(TAG, "Install panel IO");
//     esp_lcd_panel_io_handle_t io_handle = NULL;
//     esp_lcd_panel_io_i2c_config_t io_config = {
//         .dev_addr = EXAMPLE_I2C_HW_ADDR,
//         .control_phase_bytes = 1,               // According to SSD1306 datasheet
//         .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
//         .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
// #if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
//         .dc_bit_offset = 6,                     // According to SSD1306 datasheet
// #elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
//         .dc_bit_offset = 0,                     // According to SH1107 datasheet
//         .flags =
//         {
//             .disable_control_phase = 1,
//         }
// #endif
//     };
//     ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

//     ESP_LOGI(TAG, "Install SSD1306 panel driver");
//     esp_lcd_panel_handle_t panel_handle = NULL;
//     esp_lcd_panel_dev_config_t panel_config = {
//         .bits_per_pixel = 1,
//         .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
//     };
// #if CONFIG_EXAMPLE_LCD_CONTROLLER_SSD1306
//     ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
// #elif CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
//     ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &panel_handle));
// #endif

//     ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
//     ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
//     ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

// #if CONFIG_EXAMPLE_LCD_CONTROLLER_SH1107
//     ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
// #endif

//     ESP_LOGI(TAG, "Initialize LVGL");
//     const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
//     lvgl_port_init(&lvgl_cfg);

//     const lvgl_port_display_cfg_t disp_cfg = {
//         .io_handle = io_handle,
//         .panel_handle = panel_handle,
//         .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
//         .double_buffer = true,
//         .hres = EXAMPLE_LCD_H_RES,
//         .vres = EXAMPLE_LCD_V_RES,
//         .monochrome = true,
//         .rotation = {
//             .swap_xy = false,
//             .mirror_x = false,
//             .mirror_y = false,
//         }
//     };
//     lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);
//     /* Register done callback for IO */
//     const esp_lcd_panel_io_callbacks_t cbs = {
//         .on_color_trans_done = notify_lvgl_flush_ready,
//     };
//     esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

//     /* Rotation of the screen */
//     lv_disp_set_rotation(disp, LV_DISP_ROT_NONE);

//     ESP_LOGI(TAG, "Display LVGL Scroll Text");
//     //displayDishesCounted(disp);
// }

// // void displayDishesCounted(lv_disp_t *disp) {
// //     lv_obj_t *scr = lv_disp_get_scr_act(disp);
// //     lv_obj_t *label = lv_label_create(scr);
// //     lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
// //     lv_label_set_text(label, "Hello Espressif, Hello LVGL.");
// //     /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
// //     lv_obj_set_width(label, disp->driver->hor_res);
// //     lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
// // }

//  void displayDishesCounted(int count) {
//     lv_obj_t *scr = lv_disp_get_scr_act(disp);
//     lv_obj_t *label = lv_label_create(scr);
//     lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    
//     // Create a buffer to hold the text with the counted dishes
//     char text_buffer[50];
//     snprintf(text_buffer, sizeof(text_buffer), "Dishes counted: %d", count);
    
//     lv_label_set_text(label, text_buffer);
    
//     /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
//     lv_obj_set_width(label, disp->driver->hor_res);
//     lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
// }


