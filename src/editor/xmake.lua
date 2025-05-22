target("fractal")
    set_kind("binary")
    set_default(true)

    add_deps("engine", "platform", "sample_game")

    add_includedirs("..")

    add_files("main.cpp", "*.cpp", "runtime/*.cpp", "vendor/imgui/imgui_impl_bgfx.cpp")
    add_headerfiles("runtime/*.h", "panels/*.h", "systems/*.h")

    add_packages("imgui", "boost", "libsdl2", "bgfx", "glm", "libsdl2_ttf", "portaudio")

    after_build(function (target)
        os.cp("assets", target:targetdir())                       -- shaders + textures
        os.cp("src/editor/resources", path.join(target:targetdir(), "resources"))
    end)
target_end()