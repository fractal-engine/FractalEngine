--───────────────────────────────────────────────────────────
-- DEPENDENCIES
--───────────────────────────────────────────────────────────
target("FastNoise2")
    set_kind("phony")

    on_load(function (target)
        target:add("defines", "FASTNOISE2_NOISETOOL=OFF")
    end)
    
    add_includedirs("FastNoise2/include", {public = true})
    
    on_build(function (target)
        local find_tool = import("lib.detect.find_tool")
        local config = import("core.project.config")
        local cmake = find_tool("cmake")
        if cmake then
            local source_dir = path.join(os.projectdir(), "thirdparty", "FastNoise2")
            
            -- 1. Check Submodules
            if not os.isfile(path.join(source_dir, "include/FastSIMD/FastSIMD.h")) then
                os.raise("FastNoise2 submodules missing! Run: git submodule update --init --recursive")
            end

            os.cd(source_dir)

            -- 2. Clean Build Folder (Fixes the cache corruption)
            if os.exists("build") then
                os.rm("build")
            end
            
            local cmake_args = {
                "-S", ".", 
                "-B", "build",
                "-DFASTNOISE2_NOISETOOL=OFF",
                "-DCMAKE_BUILD_TYPE=Release"
            }
            
            if os.host() == "windows" then
                -- Dynamically set Visual Studio generator
                local vs_version = config.get("vs") -- Returns "2022", "2019", "2026", etc.
                local generator = "Visual Studio 17 2022" -- Safe Default
                
                if vs_version == "2026" or vs_version == "18.0" then
                    generator = "Visual Studio 18 2026"
                elseif vs_version == "2022" or vs_version == "17.0" then
                    generator = "Visual Studio 17 2022"
                elseif vs_version == "2019" or vs_version == "16.0" then
                    generator = "Visual Studio 16 2019"
                end

                print("Detected VS: " .. tostring(vs_version) .. " -> Using Generator: " .. generator)

                table.insert(cmake_args, "-G")
                table.insert(cmake_args, generator)
                table.insert(cmake_args, "-A")
                table.insert(cmake_args, "x64")

                -- Modern MT Flags
                table.insert(cmake_args, "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW")
                table.insert(cmake_args, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
            end
            
            os.execv(cmake.program, cmake_args)
            os.execv(cmake.program, {"--build", "build", "--config", "Release"})
        end
    end)

    if is_plat("windows") then
        add_linkdirs("FastNoise2/build/src/Release", {public = true})
    else
        add_linkdirs("FastNoise2/build/src", {public = true})
    end
    
    add_links("FastNoise", {public = true})
target_end()