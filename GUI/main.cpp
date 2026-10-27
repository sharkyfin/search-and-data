#include <iostream>
#include <stdexcept>
#include <string_view>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

#include "GUI/Application.hpp"
#include "GUI/Window.hpp"

int main(int argc, char** argv) {
    try {
        const bool screenshot = (argc == 3 || argc == 4) && std::string_view(argv[1]) == "--screenshot";
        const std::string_view view = argc == 4 ? argv[3] : "index";
        if ((argc != 1 && !screenshot) || (view != "index" && view != "benchmark")) {
            std::cerr << "Запуск: indexing_ui [--screenshot path.png [index|benchmark]]\n";
            return 1;
        }
        Window window;
        Application application(window.GetFont());
        if (screenshot) application.PreparePreview(view == "benchmark");
        int frames = 0;
        while (!WindowShouldClose()) {
            BeginDrawing();
            application.Draw();
            EndDrawing();
            if (application.IsBusy()) frames = 0;
            if (screenshot && !application.IsBusy() && ++frames == 4) {
                Image image = LoadImageFromScreen();
                const bool saved = ExportImage(image, argv[2]);
                UnloadImage(image);
                if (!saved) throw std::runtime_error("Не удалось сохранить снимок окна.");
                break;
            }
        }
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
    return 0;
}
