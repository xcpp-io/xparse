target("cli")
    add_deps("xparse")

    set_kind("binary")
    set_basename("xparse")
    add_files("**.cpp")