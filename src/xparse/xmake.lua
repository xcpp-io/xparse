target("xparse")
    set_kind("headeronly")
    add_packages("libtooling", { public = true })
    add_includedirs("..", { public = true })