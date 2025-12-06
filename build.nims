#!/usr/bin/env -S nim --hints:off

#
# nimscript to build the system.
#
# Usage: nim build build.nims
#

import std/os
import std/distros
import std/tables
import std/strformat

mode = ScriptMode.Verbose

let sdir = thisDir()

let cmake = findExe("cmake")
let ctest = findExe("ctest")

let CMAKE_BUILD_TYPE = "Release"

if detectOs(Windows):
    foreignDep "choco.exe"
    foreignDep "msys2.exe"

if defined(windows):
    echo "[i] OS is windows"
    putEnv("PATH", "/mingw64/bin:" & getEnv("PATH"))

proc build_cmake(buildDir, buildType, sourceDir = "..",
    force_reconfigure = true) =
    mkdir(builddir)
    withDir(buildDir):
        echo fmt"Building in {builddir}";
        if not fileExists("CMakeCache.txt") or force_reconfigure:
            exec(fmt"{cmake} {sourceDir} -DCMAKE_BUILD_TYPE={buildType} -G ""Visual Studio 16 2019""")
        exec(fmt"{cmake} --build . --config {buildType}")
        exec(fmt"{ctest} -C {buildType}")

task setupCI, "Setup CI":
    echo "[i] Setting up CI"

task build_minifilter, "build minifilter":
    echo fmt">> Building minifilter using {cmake}"
    let srcDir = sdir / "minifilter"
    build_cmake(srcDir / "build", "Release", srcDir)

task build_app, "build app":
    echo fmt">> Building app using {cmake}"
    let srcDir = sdir / "app"
    build_cmake(srcDir / "build", "Release", srcDir, force_reconfigure = false)


task build, "Build application":
    build_minifilterTask()
    build_appTask()

