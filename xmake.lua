set_languages("c++20")
add_cxxflags("/Zc:__cplusplus")
add_cxxflags("/Zc:preprocessor")
add_requires("ftxui", "boost", "libsdl2", "libsdl2_ttf", "portaudio", "glm")
add_requires("imgui 1.91.8-docking", { configs = { sdl2 = true, sdl2_renderer = true, docking = true} })

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
    add_files("src/core/*.cpp")  -- Add source files from base
    add_files("src/renderer/*.cpp")
    add_files("src/renderer/shaders/*.cpp")
    add_files("src/subsystem/input/*.cpp")  -- Add source files from base
    add_files("src/game/*.cpp")  -- Add source files from game
    add_files("src/editor/*.cpp")  -- Add source files from editor
    add_files("src/tools/*.cpp")  -- Add ImGuiBackend, BGFX drivers
    add_files("src/audio/*.cpp")
    add_files("src/scene/*.cpp")
    add_files("src/lighting/*.cpp")
    add_files("src/editor/vendor/imgui/imgui_impl_bgfx.cpp")
    add_files("src/shaders/**.sc", {rule = "bgfx_shaderc"}) 
    add_files("src/platform/platform_utils.cpp")

    -- Add Metal frameworks for macOS
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
        add_files("src/platform/*.mm")
        add_files("src/platform/*.cpp")
        add_includedirs("src")
    end

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
    add_packages("ftxui", "boost", "libsdl2", "libsdl2_ttf", "imgui", "portaudio", "bgfx", "glm") -- Add packages

    after_build(function (target)
        os.cp("assets/shaders/**", path.join(target:targetdir(), "assets/shaders"))
        os.cp("audio_lib", target:targetdir())
        os.cp("src/editor/resource/fonts/NotoSansMono_Regular.ttf", target:targetdir())
        os.cp("src/editor/resource/fonts/fa-solid-900.ttf", target:targetdir())
        os.cp("src/editor/resource/fonts/TerminusTTF-4.49.3.ttf", target:targetdir())
        os.cp("src/editor/resource/fonts/moder-dos-437.ttf", target:targetdir())
        os.cp("src/textures", path.join(target:targetdir(), "assets"))


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
            "-DCMAKE_USE_RELATIVE_PATHS=ON",
            "-DBGFX_CUSTOM_TARGETS=ON", 
            "-DBGFX_BUILD_RENDERER_DIRECT3D12=ON",
            "-DBX_CONFIG_DEBUG=" .. (package:debug() and "1" or "0")
        }

        -- Add platform-specific configurations
        if is_plat("windows") then
            table.insert(configs, "-DBGFX_BUILD_RENDERER_DIRECT3D11=ON")
            table.insert(configs, "-DBGFX_BUILD_RENDERER_DIRECT3D12=ON")
        elseif is_plat("macosx") then
            table.insert(configs, "-DBGFX_BUILD_RENDERER_METAL=ON")
        elseif is_plat("linux") then
            table.insert(configs, "-DBGFX_BUILD_RENDERER_VULKAN=ON")
        end
        
        -- Common renderers
        table.insert(configs, "-DBGFX_BUILD_RENDERER_OPENGL=ON")
        
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")

        import("package.tools.cmake").install(package, configs)
    end)
package_end()
-----------------------------------------------------------------------------------------------------------------
-- Cleaned-up and fixed version of the xmake rule for compiling BGFX shaders

rule("bgfx_shaderc")
    set_extensions(".sc")

    on_build_file(function (target, sourcefile)
        -- Use is_host() to determine separator safely in xmake's sandboxed Lua
        local path_sep = is_host("windows") and "\\" or "/"

        local filename = path.filename(sourcefile)
        local normalized_sourcefile = path.normalize(sourcefile)

        print("----------------------------------------------------")
        print("Shaderc Rule Check For: " .. sourcefile)
        print("  Normalized: " .. normalized_sourcefile)
        print("  Filename: " .. filename)

        -- Exclude varying definition files
        if filename:startswith("varying") then
            print("  Skipping: Filename starts with 'varying'.")
            print("----------------------------------------------------")
            return 
        end

        -- Exclude files in "includes" directories
        local includes_pattern_middle = path_sep .. "includes" .. path_sep
        local dir_of_sourcefile = path.directory(normalized_sourcefile)

        if path.filename(dir_of_sourcefile) == "includes" then
            print("  Skipping: File is directly in an 'includes' directory ('" .. dir_of_sourcefile .. "').")
            print("----------------------------------------------------")
            return
        elseif normalized_sourcefile:find(includes_pattern_middle, 1, true) then 
            print("  Skipping: Path contains '" .. includes_pattern_middle .. "': " .. normalized_sourcefile)
            print("----------------------------------------------------")
            return
        end

        -- Ensure it's a primary shader type (vs_*, fs_*, cs_*)
        local is_vs = filename:match("^vs_.*%.sc$") 
        local is_fs = filename:match("^fs_.*%.sc$")
        local is_cs = filename:match("^cs_.*%.sc$")

        print("  Is VS? " .. (is_vs and "YES ("..is_vs..")" or "NO"))
        print("  Is FS? " .. (is_fs and "YES ("..is_fs..")" or "NO"))
        print("  Is CS? " .. (is_cs and "YES ("..is_cs..")" or "NO"))

        if not (is_vs or is_fs or is_cs) then
            print("  Skipping: Not a primary shader type (vs_*, fs_*, cs_*).")
            print("----------------------------------------------------")
            return 
        end

        print("  PROCESSING PRIMARY SHADER: " .. sourcefile)
        print("----------------------------------------------------")

        -- Determine shaderc path
        local shaderc = ""
        local projectdir = os.projectdir()
        if is_plat("windows") then
            shaderc = path.join(projectdir, "src/thirdparty/bgfx.cmake/.build/win64_vs2022/cmake/bgfx/Release/shaderc.exe")
        elseif is_plat("macosx") then
            shaderc = path.join(projectdir, "src/thirdparty/bgfx.cmake/.build/osx/cmake/bgfx/shaderc")
        elseif is_plat("linux") then
            shaderc = path.join(projectdir, "src/thirdparty/bgfx.cmake/.build/linux/bin/shaderc")
        end

        if not os.isfile(shaderc) then
            print("Error: shaderc not found at: " .. shaderc)
            return
        end

        -- Platform-specific backend configurations
        local all_backends = {
            { platform = "windows", profile = "s_5_0", folder = "dx11" },
            { platform = "windows", profile = "s_5_0", folder = "dx12" },
            { platform = "linux",   profile = "440",   folder = "glsl" },
            { platform = "osx",     profile = "metal", folder = "metal" },
            { platform = "android", profile = "spirv", folder = "spirv" }
        }

        local current_platform = ""
        if is_plat("windows") then
            current_platform = "windows"
        elseif is_plat("macosx") then
            current_platform = "osx"
        elseif is_plat("linux") then
            current_platform = "linux"
        elseif is_plat("android") then
            current_platform = "android"
        end

        local backends_to_compile = {}
        for _, backend in ipairs(all_backends) do
            if backend.platform == current_platform then
                table.insert(backends_to_compile, backend)
            end
        end

        if #backends_to_compile == 0 then
            print("Warning: No shader backends defined for platform: " .. current_platform)
            print("Using default backend (GLSL)")
            table.insert(backends_to_compile, {platform = "linux", profile = "440", folder = "glsl"})
        end

        local shader_type = sourcefile:match("vs_") and "vertex" or sourcefile:match("fs_") and "fragment" or "compute"
        -- Determine varying definition file based on sourcefile
        local varying_file = path.join(projectdir, "src/shaders/varying.def.sc")
        if sourcefile:find("imgui", 1, true) then
            varying_file = path.join(projectdir, "src/shaders/varying_imgui.def.sc")
        elseif sourcefile:find("skybox", 1, true) then
            varying_file = path.join(projectdir, "src/shaders/varying_skybox.def.sc")
        elseif sourcefile:find("terrain", 1, true) then
            varying_file = path.join(projectdir, "src/shaders/varying_terrain_pbr.def.sc")
        elseif sourcefile:find("shadow", 1, true) then
            varying_file = path.join(projectdir, "src/shaders/varying_shadow.def.sc")
        elseif sourcefile:find("water", 1, true) then
            varying_file = path.join(projectdir, "src/shaders/varying_water.def.sc")
        end

        for _, backend in ipairs(backends_to_compile) do
            local output_dir = path.join("assets/shaders", backend.folder)
            os.mkdir(output_dir)

            local args = {
                "--type", shader_type:sub(1, 1),
                "--platform", backend.platform,
                "--profile", backend.profile,
                "--varyingdef", varying_file,
                "-i", path.join(projectdir, "src/thirdparty/bgfx.cmake/bgfx/src"),
                "-i", path.join(projectdir, "src/thirdparty/bgfx.cmake/bgfx/examples/common"),
                "-i", path.join(projectdir, "src/shaders/includes"),
                "--entry", "main",
                "-f", sourcefile,
                "-o", path.join(output_dir, path.basename(sourcefile) .. ".bin")
            }

            print("Shaderc Command: " .. table.concat(args, " "))
            print("Using varying.def.sc: " .. varying_file)
            local ok, out, err = os.iorunv(shaderc, args)
            if not ok then
                print("Shaderc failed for " .. sourcefile .. ": " .. (err or out))
            end
        end
    end)
rule_end()


-----------------------------------------------------------------------------------------------------------------