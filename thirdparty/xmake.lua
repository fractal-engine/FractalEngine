--───────────────────────────────────────────────────────────
-- DEPENDENCIES
--───────────────────────────────────────────────────────────

-- FastNoise2 - handled externally via CMake
target("FastNoise2")
    set_kind("phony")

    add_defines("FASTNOISE_STATIC_LIB", {public = true})
    
    add_includedirs("FastNoise2/include", {public = true})
    add_includedirs("FastNoise2/build/_deps/fastsimd-src/include", {public = true})
    add_linkdirs("FastNoise2/build/src", {public = true})
    add_links("FastNoise", {public = true})
    
    on_build(function (target)
        local lib = path.join("$(projectdir)", "thirdparty/FastNoise2/build/src/libFastNoise.a")
        if os.isfile(lib) then return end

        import("lib.detect.find_tool")
        import("core.project.config")
        
        local cmake = find_tool("cmake")
        if cmake then
            os.cd("$(projectdir)/thirdparty/FastNoise2")
            
            local cmake_args = {
                "-B", "build",
                "-DFASTNOISE2_TOOLS=OFF",
                "-DFASTNOISE2_TESTS=OFF",
                "-DFASTNOISE2_UTILITY=OFF",
                "-DBUILD_SHARED_LIBS=OFF",
                "-DCMAKE_BUILD_TYPE=Release",
            }
            
            -- Check platform inside build function
            if os.host() == "windows" then
                -- ! MT runtime via compiler flags (required for Windows compilation!)
                table.insert(cmake_args, "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW")
                table.insert(cmake_args, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
            end
            
            os.execv(cmake.program, cmake_args)
            os.execv(cmake.program, {"--build", "build", "--config", "Release"})
        end
    end)
target_end()

target("implot")
    set_kind("static")
    
    add_files("implot/*.cpp")
    add_headerfiles("implot/*.h")
    add_includedirs("implot", {public = true})
    
    add_packages("imgui")
target_end()

target("imgui-node-editor")
    set_kind("static")

    add_files("imgui-node-editor/*.cpp")
    add_headerfiles("imgui-node-editor/*.h")
    add_includedirs("imgui-node-editor", {public = true})

    add_packages("imgui")

        on_config(function(target)
        -- ! Wrapper required due to ImGui::GetKeyIndex() being used
        -- ! in imgui-node-editor but removed in ImGui 1.87
        target:add("cxxflags", "-include", path.join(os.scriptdir(), "imgui_compat.h"), {force = true})
    end)
target_end()

target("ImGuiFileDialog")
    set_kind("static")

    add_files("ImGuiFileDialog/*.cpp")
    add_headerfiles("ImGuiFileDialog/*.h")
    add_includedirs("ImGuiFileDialog", {public = true})

    add_packages("imgui")
target_end()