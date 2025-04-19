set_project("xparse")

add_rules("mode.debug", "mode.release")
set_languages("cxx17")

includes("xmake/desc_ext.lua")

-- repos
add_repositories("xparse-repo xmake/repos")
add_requires("libtooling")

-- modules, rules and options
add_moduledirs("xmake/modules")
includes("xmake/rules.lua")
includes("xmake/options.lua")

-- targets
includes("main/xmake.lua")
includes("tests/xmake.lua")
