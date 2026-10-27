#pragma once

#include <stdexcept>

#include "raylib.h"
#include "raygui.h"

#include "UiTheme.hpp"

class Window {
private:
    Font font;
    bool customFont{false};

public:
    Window() {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(1180, 860, "Индексация");
        if (!IsWindowReady()) throw std::runtime_error("Не удалось создать графическое окно.");
        SetTargetFPS(60);
        SetExitKey(KEY_NULL);
        font = GetFontDefault();
        const char* const paths[]{
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "C:/Windows/Fonts/arial.ttf"
        };
        constexpr int codepointCount = 95 + 64 + 2 + 112;
        int codepoints[codepointCount]{};
        for (int index = 0; index < 95; ++index) {
            codepoints[index] = 32 + index;
        }
        for (int index = 0; index < 64; ++index) {
            codepoints[95 + index] = 0x410 + index;
        }
        codepoints[95 + 64] = 0x401;
        codepoints[95 + 64 + 1] = 0x451;
        for (int index = 0; index < 112; ++index) {
            codepoints[95 + 64 + 2 + index] = 0x2000 + index;
        }
        for (const char* path : paths) {
            if (FileExists(path)) {
                const Font loaded = LoadFontEx(path, 32, codepoints, codepointCount);
                customFont = loaded.texture.id != 0 && loaded.texture.id != GetFontDefault().texture.id;
                if (customFont) {
                    font = loaded;
                    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
                    break;
                }
            }
        }
        GuiSetFont(font);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, static_cast<int>(ColorToInt(UiTheme::Border)));
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, static_cast<int>(ColorToInt(WHITE)));
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, static_cast<int>(ColorToInt(UiTheme::Ink)));
        GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, static_cast<int>(ColorToInt(UiTheme::Accent)));
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, static_cast<int>(ColorToInt(UiTheme::Accent)));
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, static_cast<int>(ColorToInt(Color{242, 246, 252, 255})));
        GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, static_cast<int>(ColorToInt(UiTheme::Accent)));
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, static_cast<int>(ColorToInt(Color{232, 239, 249, 255})));
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, static_cast<int>(ColorToInt(UiTheme::Accent)));
    }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    ~Window() {
        if (customFont) {
            UnloadFont(font);
        }
        CloseWindow();
    }

    const Font& GetFont() const {
        return font;
    }
};
