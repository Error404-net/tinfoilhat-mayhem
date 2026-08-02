/*
 * Tinfoil Hat Competition — Mayhem external app registration.
 *
 * Builds to tinfoilhat.ppma. Copy it to the SD card's /APPS/ folder; it appears
 * in the Games menu. No firmware reflash required.
 */
// Include order matters: external_app.hpp must come LAST. It depends on
// app_location_t (from standalone_app.hpp) which only gets pulled in through
// ui_navigation.hpp's own include chain — so ui_navigation.hpp has to be seen
// first. Mirrors external/level/main.cpp.
#include "ui.hpp"
#include "ui_tinfoilhat.hpp"
#include "ui_navigation.hpp"
#include "external_app.hpp"

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
        // 16x16 1bpp classic tinfoil hat: pyramid cone + brim + oval head.
        // Bit order: LSB of each byte = leftmost pixel in that 8-col group.
        // Col 0-7 in byte[0], col 8-15 in byte[1].
        //
        // .......X........  single-pixel tip  (col 7)
        // ......XXX.......  3 wide            (cols 6-8)
        // .....XXXXX......  5 wide            (cols 5-9)
        // ....XXXXXXX.....  7 wide            (cols 4-10)
        // ...XXXXXXXXX....  9 wide            (cols 3-11)
        // ..XXXXXXXXXXX...  11 wide           (cols 2-12)
        // .XXXXXXXXXXXXX..  13 wide           (cols 1-13)
        // XXXXXXXXXXXXXXXX  full brim         (cols 0-15)
        // ................  gap
        // ....XXXXXXXX....  head top          (cols 4-11)
        // ...XXXXXXXXXX...  head              (cols 3-12)
        // ...XXXXXXXXXX...  head              (cols 3-12)
        // ....XXXXXXXX....  head bottom       (cols 4-11)
        // ................  (empty rows)
        // ................
        // ................
        0x80, 0x00,
        0xC0, 0x01,
        0xE0, 0x03,
        0xF0, 0x07,
        0xF8, 0x0F,
        0xFC, 0x1F,
        0xFE, 0x3F,
        0xFF, 0xFF,
        0x00, 0x00,
        0xF0, 0x0F,
        0xF8, 0x1F,
        0xF8, 0x1F,
        0xF0, 0x0F,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00},
    /*.icon_color = */ ui::Color::white().v,
    /*.menu_location = */ app_location_t::GAMES,
    /*.desired_menu_position = */ -1,

    /*.m4_app_tag = */ {'P', 'N', 'F', 'M'},  // NFM audio baseband (RSSI source)
    /*.m4_app_offset = */ 0x00000000,         // filled at compile time
};
}
