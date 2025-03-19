target("project-module-test1")
    set_default(false)
    set_kind("static")
    add_rules("c++.meta", {
        root = "include"
    })
    add_files("source/*.cpp")
