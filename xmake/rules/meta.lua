rule("c++.meta")
    on_config(function (target)
        import("xcpp.meta").setup(target, target:extraconf("rules", "c++.meta"))
    end)

    before_build(function (target)
        import("core.tool.compiler")
        import("lib.detect.find_tool")

        -- collect all header files
        local collection = "#pragma once\n"
        for _, headerfile in ipairs(target:headerfiles()) do
            local relative_path = path.relative(headerfile, target:autogendir())
            collection = collection .. "#include \"" .. relative_path .. "\"\n"
        end
        local collection_path = path.join(target:autogendir(), "collection.hpp")
        io.writefile(collection_path, collection)

        local args = {
            collection_path,
            "--root=" .. target:values("rootdir"),
            "--output=" .. target:values("metadir"),
        }

        local compilations = compiler.compflags(".cpp", { target = target })
        if target:toolchain("msvc") or target:toolchain("clang-cl") then
            table.insert(compilations, "--driver-mode=cl")
        end
        table.join2(args, "--", compilations)

        os.mkdir(target:values("metadir"))
        local out, err = os.iorunv(find_tool("xparse").program, args)
        if out and #out > 0 then
            print("┏━━━━━━━━━━━━━━━━━━[" .. target:name() .. " meta out]━━━━━━━━━━━━━━━━━━━")
            printf(out)
            print("┗━━━━━━━━━━━━━━━━━━[" .. target:name() .. " meta out]━━━━━━━━━━━━━━━━━━━")
        end
        if err and #err > 0 then
            print("┏━━━━━━━━━━━━━━━━━━[" .. target:name() .. " meta err]━━━━━━━━━━━━━━━━━━━")
            printf(err)
            print("┗━━━━━━━━━━━━━━━━━━[" .. target:name() .. " meta err]━━━━━━━━━━━━━━━━━━━")
        end
    end)
