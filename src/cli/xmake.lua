target("cli")
    add_deps("xparse")

    set_kind("binary")
    set_basename("xgen")
    add_files("**.cpp")