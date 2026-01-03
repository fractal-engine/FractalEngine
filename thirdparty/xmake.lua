--───────────────────────────────────────────────────────────
-- DEPENDENCIES
--───────────────────────────────────────────────────────────

-- FastNoise2 - handled externally via CMake
target("FastNoise2")
    set_kind("phony")

    on_load(function (target)
        target:add("defines", "FASTNOISE2_NOISETOOL=OFF", "FASTNOISE2_FETCH_IMGUI=OFF")
    end)
    
    add_includedirs("FastNoise2/include", {public = true})
    add_includedirs("FastNoise2/build/_deps/fastsimd-src/include", {public = true})
    
    on_build(function (target)
        import("lib.detect.find_tool")
        import("core.project.config")
        
        local cmake = find_tool("cmake")
        if cmake then
            os.cd("$(projectdir)/thirdparty/FastNoise2")
            
            local cmake_args = {
                "-B", "build",
                "-DFASTNOISE2_NOISETOOL=OFF",
                "-DFASTNOISE2_FETCH_IMGUI=OFF",
                "-DBUILD_SHARED_LIBS=OFF",  -- prevent DLL linkage (linker error)
            }
            
            -- Check platform inside build function
            if os.host() == "windows" then
                -- ! MT runtime via compiler flags (required for Windows compilation!)
                table.insert(cmake_args, "-DCMAKE_BUILD_TYPE=Release")
                table.insert(cmake_args, "-DCMAKE_CXX_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG")
                table.insert(cmake_args, "-DCMAKE_C_FLAGS_RELEASE=/MT /O2 /Ob2 /DNDEBUG")
                table.insert(cmake_args, "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW")
                table.insert(cmake_args, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
            else
                table.insert(cmake_args, "-DCMAKE_BUILD_TYPE=Release")
            end
            
            os.execv(cmake.program, cmake_args)
            os.execv(cmake.program, {"--build", "build", "--config", "Release"})
        end
    end)

    add_linkdirs("FastNoise2/build/Release/lib", {public = true})
    add_links("FastNoise", {public = true})
target_end()

target("implot")
    set_kind("static")
    
    add_files("implot/*.cpp")
    add_headerfiles("implot/*.h")
    add_includedirs("implot", {public = true})
    
    add_packages("imgui")
target_end()