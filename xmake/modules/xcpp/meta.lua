function setup(_target, conf)
    local full_rootdir = path.join(_target:scriptdir(), conf.root)
    if not os.isdir(full_rootdir) then
        print("[warning] rootdir not found: " .. full_rootdir)
        return
    end
    _target:set("values", "rootdir", full_rootdir)

    -- rootdir is also includedirs
    _target:add("includedirs", full_rootdir, { public = true })
    for _, header_path in ipairs(os.files(path.join(full_rootdir, "**"))) do
        _target:add("headerfiles", header_path)
    end

    _target:set("values", "metadir", path.join(os.projectdir(), _target:autogendir(), "meta"))
end
