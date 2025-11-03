--───────────────────────────────────────────────────────────
-- DEPENDENCIES
--───────────────────────────────────────────────────────────
target("FastNoise2")
    set_kind("phony")

    -- FastNoise2 ----------------------------------------
    on_load(function (target)
        import("lib.detect.find_tool")
        target:add("defines", "FASTNOISE2_NOISETOOL=OFF")
    end)
    
    add_includedirs("FastNoise2/include", {public = true})
    
    on_build(function (target)
        import("lib.detect.find_tool")
        local cmake = find_tool("cmake")
        if cmake then
            os.cd("$(projectdir)/thirdparty/FastNoise2")
            os.execv(cmake.program, {
                "-B", "build",
                "-DFASTNOISE2_NOISETOOL=OFF",
                "-DCMAKE_BUILD_TYPE=Release",
            })
            os.execv(cmake.program, {"--build", "build", "--config", "Release"})
        end
    end)
    
    -- Link built library
    add_linkdirs("FastNoise2/build/src", {public = true})
    add_links("FastNoise", {public = true})
target_end()