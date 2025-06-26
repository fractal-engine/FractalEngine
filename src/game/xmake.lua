---------------------------------------------------------------
--  game target
---------------------------------------------------------------
target("sample_game")
    set_kind("static")           -- produces *.dll / *.so / *.dylib
    add_deps("engine")

    add_includedirs("..", {public=true})
    
    add_files("*.cpp")
    add_headerfiles("*.h")

    add_packages("libsdl2", "bgfx", "boost", "imgui", "glm")
target_end()