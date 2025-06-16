---------------------------------------------------------------
--  engine target
---------------------------------------------------------------
target("engine")
    set_kind("static")

    add_deps("platform")
    add_includedirs("..", {public = true})
    add_includedirs("../../thirdparty/bgfx.cmake/bgfx/include")
    add_includedirs("../../thirdparty/bgfx.cmake/bx/include")
    add_includedirs("../../thirdparty/bgfx.cmake/bimg/include")

    add_files("core/*.cpp")
    add_files("audio/*.cpp")
    add_files("renderer/*.cpp", "renderer/lighting/*.cpp",
              "renderer/shaders/*.cpp")
    add_files("resources/*.cpp", "resources/textures/*.cpp", "scene/*.cpp")
    add_files("runtime/*.cpp")

    -- compile .sc shaders from assets with our rule
    add_files("../assets/shaders/**/**.sc", {rule = "bgfx_shaderc"})

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
    set_sourcedir("../../thirdparty/bgfx.cmake") -- Path to the bgfx source directory
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

----------------------------------------------------------------
--  bgfx_shaderc rule
----------------------------------------------------------------
rule("bgfx_shaderc")
    set_extensions(".sc")
    local built_files = {}

    on_build_file(function (target, sourcefile)

        ----------------------------------------------------------
        --  Prevent duplicate shader compilation
        ----------------------------------------------------------
        if built_files[sourcefile] then
        return -- Skip duplicate
        end
        built_files[sourcefile] = true

        ----------------------------------------------------------
        --  Path helpers / filenames
        ----------------------------------------------------------
        local sep      = is_host("windows") and "\\" or "/"
        local root     = os.projectdir()
        local filename = path.filename(sourcefile)
        local normSrc  = path.normalize(sourcefile)

        print("----------------------------------------------------")
        print("Shaderc Rule Check For: " .. sourcefile)
        print("  Normalized: " .. normSrc)
        print("  Filename:   " .. filename)

        --------------------------------------------------------------------
        --  Early-out filters
        --------------------------------------------------------------------
      if filename:startswith("varying") then
            print("  Skipping: Filename starts with 'varying'.")
            print("----------------------------------------------------")
            return
        end

        -- Exclude files in "includes" directories
        local includes_pattern_middle = sep .. "includes" .. sep
        local dir_of_sourcefile = path.directory(normSrc)

        if normSrc:find(includes_pattern_middle, 1, true) then
            print("  Skipping: Path contains '" .. includes_pattern_middle .. "': " .. normSrc)
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

        print("  PROCESSING PRIMARY SHADER " .. sourcefile)
        print("----------------------------------------------------")

        --------------------------------------------------------------------
        --  Locate shaderc executable
        --------------------------------------------------------------------
        local root = os.projectdir()
        local shaderc = ""
        if is_plat("windows") then
            shaderc = path.join(root, "thirdparty/bgfx.cmake/.build/win64_vs2022/cmake/bgfx/Release/shaderc.exe")
        elseif is_plat("macosx") then
            shaderc = path.join(root, "thirdparty/bgfx.cmake/.build/osx/cmake/bgfx/shaderc")
        elseif is_plat("linux") then
            shaderc = path.join(root, "thirdparty/bgfx.cmake/.build/linux/bin/shaderc")
        end
        if not os.isfile(shaderc) then
            print("Error: shaderc not found at: " .. shaderc)
            return
        end

        --------------------------------------------------------------------
        --  Backend matrix
        --------------------------------------------------------------------
        local matrix = {
            { platform = "windows", profile = "s_5_0", folder = "dx11" },
            { platform = "windows", profile = "s_5_0", folder = "dx12" },
            { platform = "linux",   profile = "440",   folder = "glsl" },
            { platform = "osx",     profile = "metal", folder = "metal" },
            { platform = "android", profile = "spirv", folder = "spirv" }
        }
        local current = is_plat("windows") and "windows"
                     or is_plat("macosx")  and "osx"
                     or is_plat("linux")   and "linux"
                     or is_plat("android") and "android"
                     or ""
        local backends = {}
        for _, b in ipairs(matrix) do
            if b.platform == current then table.insert(backends, b) end
        end
        if #backends == 0 then
            print("Warning: No backend for platform '" .. current .. "', default GL.")
            table.insert(backends, { platform = "linux", profile = "440", folder = "glsl" })
        end

        --------------------------------------------------------------------
        --  Pick varying files
        --------------------------------------------------------------------
        local vbase = path.join(root, "src/assets/shaders")
        local vfile = path.join(vbase, "varying.def.sc")

        if filename:find("imgui",   1, true) then
            vfile = path.join(vbase, "varying_imgui.def.sc")
        elseif filename:find("skybox", 1, true) then
            vfile = path.join(vbase, "varying_skybox.def.sc")
        elseif filename:find("terrain", 1, true) then
            vfile = path.join(vbase, "varying_terrain_pbr.def.sc")
        elseif filename:find("shadow", 1, true) then
            vfile = path.join(vbase, "varying_shadow.def.sc")
        elseif filename:find("water", 1, true) then
            vfile = path.join(vbase, "varying_water.def.sc")
        end

        --------------------------------------------------------------------
        --  Backend compilation
        --------------------------------------------------------------------
        local stype = is_vs and "v" or is_fs and "f" or "c"

        for _, b in ipairs(backends) do
            local outdir = path.join(root, "assets/shaders", b.folder)
            os.mkdir(outdir)

            local name_without_sc = path.basename(filename, ".sc")
            local output_path = path.join(outdir, name_without_sc .. ".bin")

            local args = {
                "--type", stype,
                "--platform", b.platform,
                "--profile",  b.profile,
                "--varyingdef", vfile,
                "-i", path.join(root, "thirdparty/bgfx.cmake/bgfx/src"),
                "-i", path.join(root, "thirdparty/bgfx.cmake/bgfx/examples/common"),
                "-i", path.join(root, "src/assets/shaders/includes"),
                "--entry", "main",
                "-f", sourcefile,
                "-o", output_path,
            }

            local ok, out, err = os.iorunv(shaderc, args)
            if not ok then
                print("shaderc failed for " .. sourcefile .. ": " .. (err or out))
            end
        end
    end)
rule_end()

