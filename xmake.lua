set_languages("c++17")
add_cxxflags("/Zc:__cplusplus")
add_cxxflags("/Zc:preprocessor")
add_requires("ftxui", "boost", "libsdl2", "libsdl2_ttf", "portaudio") -- Add dependencies
add_requires("imgui", {configs = {sdl2 = true, sdl2_renderer = true}})

if is_mode("debug") then
    add_defines("BX_CONFIG_DEBUG=1")
else
    add_defines("BX_CONFIG_DEBUG=0")
end

--includes("src/thirdparty/bgfx.cmake")
-- Define the path to the BGFX tools directory
local bgfx_tools_dir = path.join(bgfx_dir, ".build", "win64_vs2022", "bin")

add_rules("mode.release")

if is_mode("debug") then -- Enable Debug mode
    set_symbols("debug")
    set_optimize("none")
end
add_requires("bgfx")


-- Define 'fractal' target
target("fractal")
    set_kind("binary")

    add_includedirs("src/")
    add_includedirs("src/thirdparty/bgfx.cmake/bx/include")
    add_defines("SDL_MAIN_HANDLED")  -- Prevent SDL from redefining main()
    add_files("src/subsystem/*.cpp")  -- Add source files from subsystem
    add_files("src/*.cpp")  -- Add main source files
    add_files("src/base/*.cpp")  -- Add source files from base
    add_files("src/subsystem/input/*.cpp")  -- Add source files from base
    add_files("src/game/*.cpp")  -- Add source files from game
    add_files("src/editor/*.cpp")  -- Add source files from editor
    add_files("src/drivers/*.cpp")  -- Add ImGuiRenderer, BGFX drivers
    add_files("src/audio/*.cpp")

    -- Add shader files with slang rule
    add_files("src/shaders/**.slang", {rule = "slang_multi"})

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
         -- Add BGFX tools directory to the PATH
        local bgfx_tools_dir = path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/.build/win64_vs2022/bin")
        target:add("envs", { "PATH=" .. bgfx_tools_dir })

    end)
    add_packages("ftxui", "boost", "libsdl2", "libsdl2_ttf", "imgui", "portaudio", "bgfx") -- Add packages

    after_build(function (target)
        os.cp("audio_lib", target:targetdir()) -- Copy audio folder to build directory
        os.cp("resources/NotoSansMono_Regular.ttf", target:targetdir())
        os.cp("src/shaders/*.slang", target:targetdir() .. "/shaders")
    end)
-- Define 'display' option
option("display")
    set_default("DISPLAY_GRAPHICAL")
    set_showmenu(true)
    set_values("DISPLAY_TEXT", "DISPLAY_GRAPHICAL")

-- Define 'backend' option
option("backend")
    set_default("auto")
    set_description("Shader backend to target (e.g., spirv, glsl, metal, dx11)")
    set_showmenu(true)
    set_values("auto", "spirv", "glsl", "metal", "dx11", "dx12")
-----------------------------------------------------------------------------------------------------------------

package("bgfx")
    add_deps("cmake") -- Use CMake to build bgfx
    set_sourcedir("src/thirdparty/bgfx.cmake") -- Path to the bgfx source directory
    on_install(function (package)
        local configs = {
            "-DBGFX_BUILD_EXAMPLES=OFF",
            "-DBGFX_BUILD_TOOLS=ON",
            "-DBGFX_BUILD_RENDERER_DIRECT3D12=ON",
            "-DBX_CONFIG_DEBUG=" .. (package:debug() and "1" or "0")
        }
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF") -- Build static libraries
        table.insert(configs, "-DBGFX_BUILD_RENDERER_DIRECT3D11=ON")
        table.insert(configs, "-DBGFX_BUILD_RENDERER_DIRECT3D12=ON")
        table.insert(configs, "-DBGFX_BUILD_RENDERER_OPENGL=ON")
        import("package.tools.cmake").install(package, configs)
    end)
package_end()
-----------------------------------------------------------------------------------------------------------------
rule("slang_multi")
    set_extensions(".slang")

    on_build_file(function (target, sourcefile)
        import("core.project.config")

        -- List of backends to generate
        local backends = {
            dx11  = {target = "dxbc", profile = "sm_5_1"},
            dx12  = {target = "dxil", profile = "sm_6_0"},
            glsl  = {target = "glsl", version = "450"},
            spirv = {target = "spirv", version = "1.3"},
            metal = {target = "metal", version = "2.1"}
        }

        -- Detect shader stage
        local stage
        if sourcefile:find("vs_") then
            stage = "vertex"
        elseif sourcefile:find("fs_") then
            stage = "fragment"
        elseif sourcefile:find("compute") then
            stage = "compute"
        else
            print("Unknown shader type for: " .. sourcefile)
            return
        end

        -- Detect platform and set slangc path
        local slangc
        local projectdir = os.projectdir()

        if is_plat("windows") then
            slangc = path.join(projectdir, "src/thirdparty/slang-2025.6.3-windows-x86_64/bin/slangc.exe")
        elseif is_plat("macosx") then
            slangc = path.join(projectdir, "src/thirdparty/slang-2025.6.3-macos-x86_64/bin/slangc")
        elseif is_plat("linux") then
            slangc = path.join(projectdir, "src/thirdparty/slang/install/bin/slangc")
        end

        -- If slangc wasn't found, try checking the system path
        if not slangc or not os.isfile(slangc) then
            local slangc_in_path = try { function() return os.iorunv("which", {"slangc"}) end }
            if slangc_in_path and os.isfile(slangc_in_path:trim()) then
                slangc = "slangc"
            else
                print("Error: slangc not found. Please install it and add to PATH.")
                return
            end
        end

        -- Build command arguments
        for folderName, slangTarget in pairs(backends) do
            local outputDir = path.join("assets", "shaders", folderName)
            os.mkdir(outputDir)

            local outputFile = path.join(outputDir, path.basename(sourcefile) .. ".bin")

            local argv = {
                "-entry", "main",
                "-stage", stage,
                "-target", slangTarget.target,
            }

            -- Add profile
            if slangTarget.profile then
                table.insert(argv, "-profile")
                table.insert(argv, slangTarget.profile)
            end

            table.insert(argv, "-o")
            table.insert(argv, outputFile)
            table.insert(argv, sourcefile)

            print("Slangc Command: " .. table.concat(argv, " "))
            local ok, errors = os.iorunv(slangc, argv)
            if not ok then
                print("Failed to compile shader for backend [" .. folderName .. "]: " .. (errors or "Unknown error"))
            end
        end
    end)
rule_end()
-----------------------------------------------------------------------------------------------------------------