import("lib.detect.find_program")

function main(opt)

    opt = opt or {}
    opt.check = opt.check or "--help"
    opt.paths = opt.paths or
    {
        "$(projectdir)/tools/xcpp"
    }

    return find_program("xparse", opt)
end