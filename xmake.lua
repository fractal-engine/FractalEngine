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
    add_files("src/shaders/**.slang", {rule = "slang"})

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
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF") -- Build static libraries
        import("package.tools.cmake").install(package, configs)
    end)
package_end()
-----------------------------------------------------------------------------------------------------------------
 rule("slang")
    set_extensions(".slang")

    on_build_file(function (target, sourcefile)

    -- Set backend folder based on the target platform
        local function get_backend_folder()
            local override = get_config("backend")
            if override and override ~= "auto" then
                return override
            end

            return "spirv"
        end

        local backend = get_backend_folder()
        local asset_outputdir = path.join("assets", "shaders", backend)

        os.mkdir(asset_outputdir)

        local assetfile = path.join(asset_outputdir, path.basename(sourcefile) .. ".bin")

        -- Use platform-specific path for slangc
        local slangc = nil
        if is_plat("windows") then
            slangc = path.join(os.projectdir(), "src/thirdparty/slang-2025.6.3-windows-x86_64/bin/slangc.exe")
        elseif is_plat("macosx") then
            slangc = path.join(os.projectdir(), "src/thirdparty/slang-2025.6.3-macos-x86_64/bin/slangc")
        elseif is_plat("linux") then
            slangc = path.join(os.projectdir(), "src/thirdparty/slang-2025.6.3-linux-x86_64/bin/slangc")
        end

        -- Check if slangc exists
        if not slangc or not os.isfile(slangc) then
            local slangc_in_path = try { function() return os.iorunv("which", {"slangc"}) end }
            if slangc_in_path and os.isfile(slangc_in_path:trim()) then
                slangc = "slangc"
            else
                print("Error: slangc not found. Please install it and add to PATH.")
                return
            end
        end

        local stage = nil
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

        local target_lang_map = {
            glsl = "glsl",
            spirv = "spirv",
            metal = "metal",
            dx11 = "dx_5_0",
            dx12 = "dxil",
        }

        local slang_target = target_lang_map[backend] or backend

        local argv = {
            "-entry", "main",
            "-stage", stage,
            "-target", slang_target,
            "-o", assetfile,
            sourcefile,
        }

        print("Slangc Command: ", table.concat(argv, " "))
        os.vrunv(slangc, argv)
    end)
rule_end()
-----------------------------------------------------------------------------------------------------------------