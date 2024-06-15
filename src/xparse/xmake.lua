target("xparse")
    add_packages("libtooling")

    set_kind("static")
    add_includedirs("..", { public = true })
    add_files("**.cpp")