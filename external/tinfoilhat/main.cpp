/*
 * Tinfoil Hat Competition — Mayhem external app registration.
 *
 * Builds to tinfoilhat.ppma. Copy it to the SD card's /APPS/ folder; it appears
 * in the RX menu. No firmware reflash required.
 */
#include "external_app.hpp"
#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_tinfoilhat.hpp"

namespace ui::external_app::tinfoilhat {
void initialize_app(ui::NavigationView& nav) {
    nav.push<TinfoilHatMenuView>();
}
}  // namespace ui::external_app::tinfoilhat

extern "C" {

__attribute__((section(".external_app.app_tinfoilhat.application_information"), used))
application_information_t _application_information_tinfoilhat = {
    /*.memory_location = */ (uint8_t*)0x00000000,
    /*.externalAppEntry = */ ui::external_app::tinfoilhat::initialize_app,
    /*.header_version = */ CURRENT_HEADER_VERSION,
    /*.app_version = */ VERSION_MD5,

    /*.app_name = */ "Tinfoil Hat",
    /*.bitmap_data = */
    {
        // 16x16 1bpp "hat" glyph — a simple foil cap over a head.
        0x00, 0x00,
        0x80, 0x01,
        0xC0, 0x03,
        0xE0, 0x07,
        0xF0, 0x0F,
        0xF8, 0x1F,
        0xFC, 0x3F,
        0xFE, 0x7F,
        0xFF, 0xFF,
        0x00, 0x00,
        0xFC, 0x3F,
        0xFE, 0x7F,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00},
    /*.icon_color = */ ui::Color::grey().v,
    /*.menu_location = */ app_location_t::RX,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'N', 'F', 'M'},  // NFM audio baseband (RSSI source)
    /*.m4_app_offset = */ 0x00000000,         // filled at compile time
};
}
