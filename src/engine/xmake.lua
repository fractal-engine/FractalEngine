---------------------------------------------------------------
--  engine target
---------------------------------------------------------------
target("engine")
    set_kind("static")

    add_deps("platform")
    add_includedirs("..", {public = true})

    add_files("core/*.cpp")
    add_files("audio/*.cpp")
    add_files("renderer/*.cpp", "renderer/lighting/*.cpp",
              "renderer/shaders/*.cpp")
    add_files("resources/*.cpp", "resources/textures/*.cpp", "scene/*.cpp")
    add_files("runtime/*.cpp")

    add_rules("shaderc.build")
    add_files("$(projectdir)/src/assets/shaders/**.sc")
    remove_files("$(projectdir)/src/assets/shaders/varying*.sc")

    add_packages("boost", "libsdl2", "bgfx", "glm", "imgui", "libsdl2_ttf", "portaudio")

    if is_mode("debug") then
    add_links("bimg_decodeDebug", "bimg_encodeDebug")
    else
        add_links("bimg_decodeRelease", "bimg_encodeRelease")
    end
    add_defines("BGFX_STATIC_LIB")

     -- macOS specific frameworks that BGFX needs
    if is_plat("macosx") then
        add_frameworks("Metal", "MetalKit", "QuartzCore")
    end
target_end()


--───────────────────────────────────────────────────────────
--  SHADERC RULE – self-contained shader compiler
--───────────────────────────────────────────────────────────
rule("shaderc.build")
    set_extensions(".sc", ".vert", ".frag", ".comp")

    before_buildcmd_file(function (target, batchcmds, shaderfile, opt)
        ---------------------------------------------------------- shaderc exe
        local exe = target:data("shaderc_exe")
        if not exe then
            import("lib.detect.find_program")
            exe =  find_program("shaderc")
                or find_program("shadercRelease")
                or find_program("shadercDebug")
            if not exe then                 -- search inside bgfx package
                local bgfx = target:pkg("bgfx")
                if bgfx then
                    local bin = bgfx:installdir("bin")
                    exe =  find_program("shaderc",        {paths = bin})
                        or find_program("shadercRelease", {paths = bin})
                        or find_program("shadercDebug",   {paths = bin})
                end
            end
            assert(exe, "shaderc executable not found; rebuild bgfx with tools=true")
            target:data_set("shaderc_exe", exe)
        end

        --------------------------------------------------- skip non-shader files
        local fname = path.filename(shaderfile)           
        if fname:startswith("varying") then return end

        local stype =
              fname:match("^vs_") and "vertex"
           or fname:match("^fs_") and "fragment"
           or fname:match("^cs_") and "compute"
        if not stype then return end                      

        ---------------------------------------------------------------- varying files
        local shader_root = path.join(os.projectdir(), "src/assets/shaders")

        -- default
        local varying = path.join(shader_root, "varying.def.sc")

        -- dispatch table – extend for new varying files
        local map = {
            imgui   = "varying_imgui.def.sc",
            skybox  = "varying_skybox.def.sc",
            terrain = "varying_terrain_pbr.def.sc",
            shadow  = "varying_shadow.def.sc",
            water   = "varying_water.def.sc",
        }

        -- pick first key matching anywhere in the relative path
        local rel = path.relative(shaderfile, shader_root)
        for key, file in pairs(map) do
            if rel:find(key, 1, true) then
                varying = path.join(shader_root, file)
                break
            end
        end

        assert(os.isfile(varying), "No varying file for shader '" .. fname ..
            "'\n  selected: " .. varying)


        ------------------------------------------------------------- output bin
        local binname = fname:gsub("%.sc$", ".bin") 

        local plat    = is_plat("macosx") and "osx"
                    or is_plat("windows") and "windows"
                    or                       "linux"

        local backend = ({osx = "metal", windows = "dx11", linux = "glsl"})[plat]

        -- write inside build directory (…/.build/<plat>/<arch>/<mode>)
        local outdir  = path.join(target:targetdir(), "assets/shaders", backend)
        batchcmds:mkdir(outdir)

        local outfile = path.join(outdir, binname)       -- final full path

        ------------------------------------------------------------- build args
        local profile = (plat=="osx") and "metal"
                     or (plat=="windows") and "s_5_0"
                     or (stype=="compute") and "430" or "120"

        local args = {
            "--platform", plat, "--type", stype, "--profile", profile,
            "--varyingdef", varying,
            "-i", path.join(os.projectdir(), "thirdparty/bgfx_helpers/common"),
            "-i", path.join(os.projectdir(), "thirdparty/bgfx_helpers/src"),
            "-f", shaderfile, "-o", outfile,
        }

        batchcmds:show_progress(opt.progress,
            "${color.build.object}shaderc %s", fname)
        batchcmds:vrunv(exe, args)
        batchcmds:set_depmtime(os.mtime(outfile))
        batchcmds:add_depfiles(shaderfile, varying)
    end)

