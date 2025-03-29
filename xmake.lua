set_languages("c++17")

add_requires("ftxui", "boost", "libsdl3", "libsdl3_ttf", "portaudio")
add_requires("imgui", {configs = {sdl3 = true, sdl3_renderer = true}})

add_rules("mode.release")

if is_mode("debug") then -- Enable Debug mode
    set_symbols("debug")
    set_optimize("none")
end

-- Define 'shard-cpp' target
target("fractal")
    set_kind("binary")

    add_includedirs("src/")
    add_defines("SDL_MAIN_HANDLED")  -- Prevent SDL from redefining main()
    add_files("src/subsystem/*.cpp")  -- Add source files from subsystem
    add_files("src/*.cpp")  -- Add main source files
    add_files("src/base/*.cpp")  -- Add source files from base
    add_files("src/subsystem/input/*.cpp")  -- Add source files from base
    add_files("src/game/*.cpp")  -- Add source files from game
    add_files("src/editor/*.cpp")  -- Add source files from editor
    add_files("src/audio/*.cpp")
    add_options("display")
    on_load(function (target)
        local display_mode = get_config("display")
        if display_mode == "DISPLAY_GRAPHICAL" then
            target:add("defines", "DISPLAY_GRAPHICAL")
        else
            target:add("defines", "DISPLAY_TEXT")
        end

        local pkgs = target:pkgs("imgui")
        if pkgs and #pkgs > 0 then
            local pkg = pkgs[1]
            target:add("includedirs", pkg:installdir() .. "/include", {public = true})
        end
    end)
    add_packages("ftxui", "boost", "libsdl3", "libsdl3_ttf", "imgui", "portaudio")
    after_build(function (target)
        os.cp("audio_lib", target:targetdir()) -- Copy audio folder to build directory
        os.cp("resources/NotoSansMono_Regular.ttf", target:targetdir())
    end)
-- Define 'display' option
option("display")
    set_default("DISPLAY_TEXT")
    set_showmenu(true)
    set_values("DISPLAY_TEXT", "DISPLAY_GRAPHICAL")
