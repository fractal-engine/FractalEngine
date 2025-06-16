---------------------------------------------------------------
--  editor target
---------------------------------------------------------------
target("fractal")
    set_kind("binary")
    set_default(true)

    add_deps("engine", "platform", "sample_game")

    add_includedirs("..")

    add_files("main.cpp", "*.cpp", "runtime/*.cpp", "vendor/imgui/imgui_impl_bgfx.cpp")
    add_headerfiles("runtime/*.h", "panels/*.h", "systems/*.h")

    add_packages("imgui", "boost", "libsdl2", "bgfx", "glm", "libsdl2_ttf", "portaudio")

    -- copy all assets 
    after_build(function (target)
        local bindir = target:targetdir()

        -- Shaders
        local shaders_target = path.join(bindir, "assets/shaders")
        os.rm(shaders_target)
        os.cp("assets/shaders", shaders_target)

        -- Textures
        local textures_target = path.join(bindir, "assets/textures")
        os.rm(textures_target)
        os.cp("src/assets/textures", textures_target)

        -- Audio
        local audio_target = path.join(bindir, "assets/audio")
        os.rm(audio_target)
        os.cp("src/assets/audio", audio_target)

        -- Editor Resources
        local res_target = path.join(bindir, "resources")
        os.rm(res_target)
        os.cp("src/editor/resources", res_target)


    end)
    add_defines("SDL_MAIN_HANDLED", "DISPLAY_GRAPHICAL")
target_end()