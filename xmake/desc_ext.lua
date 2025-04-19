function target_component(owner, name)
    local component_name = owner .. "." .. name
    target(owner)
        add_deps(component_name)
        add_values("components", component_name)
    target_end()

    target(component_name)
        set_default(false)
        set_policy("build.fence", true)
        set_values("ownername", owner)
        set_values("component", name)
end
