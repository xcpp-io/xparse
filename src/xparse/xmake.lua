target("xparse")
    add_packages("mustache", "libtooling", "lua")

    set_kind("static")
    add_includedirs("..", { public = true })
    add_files("**.cpp")