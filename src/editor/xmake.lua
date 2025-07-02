---------------------------------------------------------------
--  editor target
---------------------------------------------------------------
target("fractal")
    set_kind("binary")
    set_default(true)

    add_deps("engine", "platform", "sample_game")

    add_includedirs("..", "vendor", "systems")

    add_files("main.cpp", "*.cpp", "runtime/*.cpp", "panels/*.cpp", "systems/*.cpp", "vendor/imgui/imgui_impl_bgfx.cpp", "vendor/ImGuiFileDialog/ImGuiFileDialog.cpp")
    add_headerfiles("runtime/*.h", "panels/*.h", "systems/*.h")

    add_packages("imgui", "boost", "libsdl2", "bgfx", "glm", "libsdl2_ttf", "portaudio")

    -- copy all assets 
    after_build(function (target)
        local bindir = target:targetdir()
        os.cp("src/assets/textures", path.join(bindir, "assets/textures"))
        os.cp("src/assets/audio",    path.join(bindir, "assets/audio"))
        os.cp("src/editor/resources/icons", path.join(bindir, "resources/icons"))
        os.cp("src/editor/resources/fonts",   path.join(bindir, "resources/fonts"))
    end)
    add_defines("SDL_MAIN_HANDLED", "DISPLAY_GRAPHICAL")
target_end()