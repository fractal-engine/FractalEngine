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
    add_files("src/scene/*.cpp")
    add_files("src/shaders/**.sc|varying.def.sc|varying_imgui.def.sc", {rule = "bgfx_shaderc"})

    -- Add Metal frameworks for macOS
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
        add_files("src/platform/*.mm")
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
    add_packages("ftxui", "boost", "libsdl2", "libsdl2_ttf", "imgui", "portaudio", "bgfx") -- Add packages

    after_build(function (target)
        os.cp("assets/shaders/**", path.join(target:targetdir(), "assets/shaders"))
        os.cp("audio_lib", target:targetdir()) -- Copy audio folder to build directory
        os.cp("resources/NotoSansMono_Regular.ttf", target:targetdir())
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
rule("bgfx_shaderc")
    set_extensions(".sc")

    on_build_file(function (target, sourcefile)
        -- Get the correct path to shaderc based on platform
        local shaderc = ""
        if is_plat("windows") then
            shaderc = path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/.build/win64_vs2022/cmake/bgfx/Release/shaderc.exe")
        elseif is_plat("macosx") then
            shaderc = path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/.build/osx/cmake/bgfx/shaderc")
        elseif is_plat("linux") then
            shaderc = path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/.build/linux/bin/shaderc")
        end
        
        if not os.isfile(shaderc) then
            print("Error: shaderc not found at: " .. shaderc)
            return
        end
        
        -- Define all possible backends
        local all_backends = {
            { platform = "windows", profile = "s_5_0", folder = "dx11" },
            { platform = "windows", profile = "s_5_0", folder = "dx12" },
            { platform = "linux",   profile = "440",   folder = "glsl" },
            { platform = "osx",     profile = "metal", folder = "metal" },
            { platform = "android", profile = "spirv", folder = "spirv" }
        }
        
        -- Get current platform identifier
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
        
        -- Filter backends for current platform only
        local backends_to_compile = {}
        for _, backend in ipairs(all_backends) do
            if backend.platform == current_platform then
                table.insert(backends_to_compile, backend)
            end
        end
        
        -- If no backends found for this platform, warn and use defaults
        if #backends_to_compile == 0 then
            print("Warning: No shader backends defined for platform: " .. current_platform)
            print("Using default backend (GLSL)")
            table.insert(backends_to_compile, {platform = "linux", profile = "440", folder = "glsl"})
        end
        
        local shader_type = sourcefile:match("vs_") and "vertex" or sourcefile:match("fs_") and "fragment" or "compute"

        local is_imgui = sourcefile:find("imgui") ~= nil
        local varying_file = is_imgui
            and path.join(os.projectdir(), "src/shaders/varying_imgui.def.sc")
            or path.join(os.projectdir(), "src/shaders/varying.def.sc")

        -- Only compile for backends of the current platform
        for _, backend in ipairs(backends_to_compile) do
            local output_dir = path.join("assets/shaders", backend.folder)
            os.mkdir(output_dir)

            local args = {
                "--type", shader_type:sub(1, 1),
                "--platform", backend.platform,
                "--profile", backend.profile,
                "--varyingdef", varying_file,
                "-i", path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/bgfx/src"),
                "-i", path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/bgfx/examples/common"),
                "--entry", "main",
                "-f", sourcefile,
                "-o", path.join(output_dir, path.basename(sourcefile) .. ".bin")
            }

            print("Shaderc Command: " .. table.concat(args, " "))
            local ok, out, err = os.iorunv(shaderc, args)
            if not ok then
                print("Shaderc failed for " .. sourcefile .. ": " .. (err or out))
            end
        end
    end)
rule_end()
-----------------------------------------------------------------------------------------------------------------