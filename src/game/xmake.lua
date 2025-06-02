---------------------------------------------------------------
--  game target
---------------------------------------------------------------
target("sample_game")
    set_kind("static")           -- produces *.dll / *.so / *.dylib
    add_deps("engine")

    add_includedirs("..", {public=true})
    add_includedirs("../../thirdparty/bgfx.cmake/bgfx/include")
    add_includedirs("../../thirdparty/bgfx.cmake/bx/include")
    add_includedirs("../../thirdparty/bgfx.cmake/bimg/include")
    
    add_files("*.cpp")
    add_headerfiles("*.h")

    add_packages("libsdl2", "bgfx", "boost", "imgui")
target_end()