target("project-module-test1")
    set_default(false)
    set_kind("static")
    add_includedirs("include", { public = true })
    add_files("source/*.cpp")

target_component("project-module-test1", "autogen")
    set_kind("headeronly")
    add_rules("c++.meta")
    add_files("include/**.h")
