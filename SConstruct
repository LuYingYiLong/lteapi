#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")
if "/std:c++17" in env.get("CXXFLAGS", []):
    env["CXXFLAGS"].remove("/std:c++17")
env.Append(CXXFLAGS=["/std:c++20", "/await:strict"])

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/", "src/plugin/", "src/server/", "src/resource/", "src/editor/", "src/util/", "src/classes/", "src/controls/", "src/helpers/"])
sources = Glob("src/*.cpp") + Glob("src/plugin/*.cpp") + Glob("src/server/*.cpp") + Glob("src/resource/*.cpp") + Glob("src/editor/*.cpp") + Glob("src/util/*.cpp") + Glob("src/classes/*.cpp") + Glob("src/controls/*.cpp") + Glob("src/helpers/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/lteapi/bin/liblteapi.{}.{}.framework/lteapi.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/lteapi/bin/liblteapi.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/lteapi/bin/liblteapi.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "addons/lteapi/bin/lteapi{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
