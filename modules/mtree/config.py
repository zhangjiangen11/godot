def can_build(env, platform):
    # glslang is only needed when Vulkan, Direct3D 12 or Metal-based renderers are available,
    # as OpenGL doesn't use glslang.
    return True


def configure(env):
    pass
