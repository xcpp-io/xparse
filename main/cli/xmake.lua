target("cli")
    set_basename("xparse")
    set_kind("binary")
    add_deps("xparse-base")
    add_files("**.cpp")

    after_build(function (target)
        os.cp(target:targetfile(), path.join(os.projectdir(), "tools"))
    end)
