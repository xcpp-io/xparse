package("libtooling")

    set_kind("library")
    set_homepage("https://llvm.org/")
    set_description("The Clang Libtooling")

    if is_plat("windows") then
        if is_arch("x86") then
            set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-x86.7z")
            add_versions("17.0.6", "b86acaef58ef2f9f8b0bae18707ef5ef29ccd86775d9b37b5ff54ff7d3751aae")
        else
            set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-x64.7z")
            add_versions("17.0.6", "8a9548e5970e580ed2096747063458b255b800b4a8fb7f6eade05dd983148150")
        end
    end

    on_load(function (package)
        package:add("syslinks", "Version", "advapi32", "Shcore", "user32", "shell32", "Ole32", { public = true })
        package:add("linkdirs", "lib")
        for _, lib_path in ipairs(os.files(package:installdir("lib"))) do
            local lib = string.gsub(path.basename(lib_path), "%..*$", "")
            package:add("links", lib)
        end
    end)

    on_install(function (package)
        os.cp("*", package:installdir())
    end)
