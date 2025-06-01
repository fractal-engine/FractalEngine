---------------------------------------------------------------
--  engine target
---------------------------------------------------------------
target("engine")
    set_kind("static")

    add_deps("platform")
    add_includedirs("..", {public = true})
    add_includedirs("../thirdparty/bgfx.cmake/bgfx/include")
    add_includedirs("../thirdparty/bgfx.cmake/bx/include")
    add_includedirs("../thirdparty/bgfx.cmake/bimg/include")

    add_files("core/*.cpp")
    add_files("audio/*.cpp")
    add_files("renderer/*.cpp", "renderer/lighting/*.cpp",
              "renderer/shaders/*.cpp")
    add_files("resources/*.cpp", "resources/textures/*.cpp", "scene/*.cpp")
    add_files("runtime/*.cpp")

    -- compile .sc shaders from assets with our rule
    add_files("../assets/shaders/**/vs_*.sc")
    add_files("../assets/shaders/**/fs_*.sc")
    add_rules("bgfx_shaderc")

    add_packages("boost", "libsdl2", "bgfx", "glm", "imgui", "libsdl2_ttf", "portaudio")

     -- macOS specific frameworks that BGFX needs
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
    end
target_end()

---------------------------------------------------------------
--  BGFX third-party package
---------------------------------------------------------------
package("bgfx")
    add_deps("cmake") -- Use CMake to build bgfx
    set_sourcedir("src/thirdparty/bgfx.cmake") -- Path to the bgfx source directory
    on_install(function (package)
        local configs = {
            "-DBGFX_BUILD_EXAMPLES=OFF",
            "-DBGFX_BUILD_TOOLS=ON",
            "-DBX_INSTALL=ON",
            "-DBIMG_INSTALL=ON",
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

        local libdir   = path.join(package:installdir(), "lib")
        local cfg_suf  = package:debug() and "Debug" or "Release"

        package:add("linkdirs", libdir)
        package:add("links", "bx", "bimg", "bgfx")
    end)
package_end()

---------------------------------------------------------------
--  bgfx_shaderc rule
---------------------------------------------------------------
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

        -- Shader definition files
        local varying_base = path.join(os.projectdir(), "src/assets/shaders")
        local varying_file = path.join(varying_base, "varying.def.sc")
        if sourcefile:find("imgui")  then
            varying_file = path.join(varying_base, "varying_imgui.def.sc")
        elseif sourcefile:find("skybox") then
            varying_file = path.join(varying_base, "varying_skybox.def.sc")
        end

        -- Only compile for backends of the current platform
        for _, backend in ipairs(backends_to_compile) do
            local output_dir = path.join(os.projectdir(), "assets/shaders", backend.folder)
            os.mkdir(output_dir)

            local args = {
                "--type", shader_type:sub(1, 1),
                "--platform", backend.platform,
                "--profile", backend.profile,
                "--varyingdef", varying_file,
                "-i", path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/bgfx/src"),
                "-i", path.join(os.projectdir(), "src/thirdparty/bgfx.cmake/bgfx/examples/common"),
                "-i", path.join(os.projectdir(), "src/assets/shaders/includes"),
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
