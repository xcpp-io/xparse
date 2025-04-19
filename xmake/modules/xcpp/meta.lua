import("core.tool.compiler")
import("core.project.project")
import("lib.detect.find_tool")

function __get_project_autogendir()
    return path.join(os.projectdir(), get_config("buildir"), ".xcpp")
end

function setup(target)
    local full_autogendir = path.join(__get_project_autogendir(), target:values("ownername"))
    target:set("values", "autogendir", full_autogendir)
    os.mkdir(full_autogendir)
end

function process(target)
    -- collect all header files
    local collection = "#pragma once\n"

    local autogen_sourcebatches = target:sourcebatches()["c++.meta"]
    if not autogen_sourcebatches then
        raise("no file specified for %s, parsing ended.", ownername)
    end
    for _, headerfile in ipairs(autogen_sourcebatches.sourcefiles) do
        local relative_path = path.relative(headerfile, target:values("autogendir"))
        collection = collection .. "#include \"" .. relative_path .. "\"\n"
    end
    local collection_path = path.join(target:values("autogendir"), "collection.hpp")
    io.writefile(collection_path, collection)

    local args = { collection_path }

    local compilations = compiler.compflags(".cpp", { target = target })
    if target:toolchain("msvc") or target:toolchain("clang-cl") then
        table.insert(compilations, "--driver-mode=cl")
    end
    table.join2(args, "--", compilations)

    local out, err = os.iorunv(find_tool("xparse").program, args)
    if err and #err > 0 then
        print("┏━━━━━━━━━━━━━━━━━━[" .. target:values("ownername") .. " log]━━━━━━━━━━━━━━━━━━━")
        printf(err)
        print("┗━━━━━━━━━━━━━━━━━━[" .. target:values("ownername") .. " log]━━━━━━━━━━━━━━━━━━━")
    end

    io.writefile(path.join(target:values("autogendir"), "meta.json"), out)
end

function clean(target)
    os.tryrm(target:values("autogendir"))

    local project_autogendir = __get_project_autogendir()
    if #os.filedirs(path.join(project_autogendir, "*")) == 0 then
        os.tryrm(project_autogendir)
    end
end
