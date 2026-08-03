#pragma once
#include <cstdint>
#include <string>

static constexpr int screen_width = 240;
static constexpr int screen_height = 320;

struct Color {
    uint8_t r{0}, g{0}, b{0};
    uint16_t v{0};
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b), v(0) {}
    static Color black()  { return {0, 0, 0}; }
    static Color white()  { return {255, 255, 255}; }
    static Color green()  { return {0, 255, 0}; }
    static Color yellow() { return {255, 255, 0}; }
    static Color red()    { return {255, 0, 0}; }
};

struct Point {
    int x{0}, y{0};
    Point() = default;
    Point(int x, int y) : x(x), y(y) {}
};

struct Size {
    int w{0}, h{0};
    Size() = default;
    Size(int w, int h) : w(w), h(h) {}
};

struct Rect {
    int x{0}, y{0}, w{0}, h{0};
    Rect() = default;
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    int left()   const { return x; }
    int top()    const { return y; }
    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    int width()  const { return w; }
    int height() const { return h; }
    Point origin() const { return {x, y}; }
};

struct Font { int char_w{8}, char_h{16}; };

class Painter {
public:
    void fill_rectangle(const Rect&, const Color&) {}
    void draw_string(const Point&, const Font&, const Color&, const Color&, const std::string&) {}
    void draw_string(const Point&, const Font&, const Color&, const Color&, const char*) {}
};

using EncoderEvent = int32_t;
enum class KeyEvent { Select, Left, Right, Up, Down, Back };

struct ThemeColors {
    Color foreground{255, 255, 255};
    Color background{0, 0, 0};
};

class Theme {
    ThemeColors light_{};
    static Theme* instance_;
public:
    ThemeColors* fg_light{&light_};
    ThemeColors* fg_medium{&light_};
    ThemeColors* fg_dark{&light_};
    static Theme* getInstance() {
        if (!instance_) instance_ = new Theme();
        return instance_;
    }
};
inline Theme* Theme::instance_ = nullptr;
