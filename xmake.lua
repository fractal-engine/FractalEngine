----------------------------------------------------------------
--  Global settings
----------------------------------------------------------------
set_languages("c++20")

set_xmakever("2.8.7")
add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

-- external dependencies
add_requires("boost", "libsdl2", "libsdl2_ttf", "portaudio", "glm", "bgfx")
add_requires("imgui 1.91.8-docking", {configs={sdl2=true, sdl2_renderer=true, docking=true}})

if is_mode("debug") then
    add_defines("BX_CONFIG_DEBUG=1")
else
    add_defines("BX_CONFIG_DEBUG=0")
end

-- macOS frameworks
if is_plat("macosx") then
    add_frameworks("Metal", "MetalKit", "QuartzCore")
end

-- copy assets next to _every_ binary
after_build(function (target)
    os.cp("assets", target:targetdir())
    os.cp("src/editor/resources", path.join(target:targetdir(),"resources"))
end)

----------------------------------------------------------------
--  per-module build scripts
----------------------------------------------------------------
includes("src/platform/xmake.lua")
includes("src/engine/xmake.lua")
includes("src/editor/xmake.lua")
includes("src/tests/xmake.lua")
includes("src/game/xmake.lua")