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
    add_includedirs("../thirdparty/bgfx.cmake/bgfx/include")
    add_includedirs("../thirdparty/bgfx.cmake/bx/include")
    add_includedirs("../thirdparty/bgfx.cmake/bimg/include")

    -- Apple frameworks for Metal + Cocoa
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
    end
target_end()