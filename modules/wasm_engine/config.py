supported_platforms = ["osx", "windows", "x11"]
supported_archs = ["x86_64", "arm64"]

def can_build(env, platform):
    if not platform in supported_platforms:
        return False
    
    if not env["arch"] in supported_archs:
        return False
    
    return True

def configure(env):
    pass
