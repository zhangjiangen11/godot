def can_build(env, platform):
    return not env["disable_3d"] and not env["arch"] == "ppc32"
    #return False


def configure(env):
    pass
