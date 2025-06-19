---------------------------------------------------------------
--  platform target
---------------------------------------------------------------
target("platform")
    set_kind("static")

    add_files("*.cpp", "input/*.cpp")
    add_headerfiles("*.h", "input/*.h")

     -- macOS-specific file
    if is_plat("macosx") then
        add_files("*.mm")
    end

    add_packages("libsdl2", "bgfx", "imgui", "boost")

    add_includedirs("..")  

    -- Apple frameworks for Metal + Cocoa
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
    end
target_end()