package("libtooling")
    set_kind("library")
    set_homepage("https://llvm.org/")
    set_description("The Clang Libtooling")

    if is_plat("windows") then
        if is_arch("x86") then
            -- set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-x86.7z")
            -- add_versions("17.0.6", "b86acaef58ef2f9f8b0bae18707ef5ef29ccd86775d9b37b5ff54ff7d3751aae")
            set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-msvc-x86-mt-release.7z")
            add_versions("18.1.8", "cf294b215772403f062989014e51cbe5d6c1e4ff87c6fe02bb10bd71945210c0")
        else
            -- set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-x64.7z")
            -- add_versions("17.0.6", "8a9548e5970e580ed2096747063458b255b800b4a8fb7f6eade05dd983148150")
            set_urls("https://github.com/xcpp-io/llvm-build/releases/download/llvm-$(version)/llvm-$(version)-msvc-x64-mt-release.7z")
            add_versions("18.1.8", "bbb0e110b007d8cc3b98659789f5db951cf151353ea9530c795f9b65b241aba2")
        end
    end

    add_syslinks("Version", "ntdll", "Ws2_32", "advapi32", "Shcore", "user32", "shell32", "Ole32", { public = true })

    on_install(function (package)
        os.cp("*", package:installdir())
    end)
