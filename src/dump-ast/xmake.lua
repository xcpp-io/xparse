target("dump-ast")
    set_kind("binary")
    add_packages("libtooling", { public = true })
    add_files("**.cpp")