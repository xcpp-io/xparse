set_project("xparse")

add_rules("mode.debug", "mode.release")
set_languages("cxx17")

add_repositories("xparse-repo xmake/repos")
add_requires("libtooling")

includes("src/xparse", "src/cli")