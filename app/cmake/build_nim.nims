#
# Wrapper script around nim to build nim code from cmake.
#
#

import std/strformat
import std/strutils
import std/sequtils
import std/os

var args : seq[string] = commandLineParams();
args.del(0);

putEnv("CC", "/mingw64/bin/gcc.exe")
let nimexec = getCurrentCompilerExe()
let strargs = join(args, " ")
let cmd = fmt"{nimexec} c --cc:env -d:mingw {strargs}"

echo fmt"Executing {cmd}"
exec(fmt"{cmd}")
