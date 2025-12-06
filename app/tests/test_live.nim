# Test live running Shield.

import std/os
import std/strformat
import std/logging

# let chars = {'a' .. 'z', 'A' .. 'Z'}

let PREFIX = ".^^^."
let POSTFIX = ".^^^"

var consoleLog = newConsoleLogger(lvlAll)
addHandler(consoleLog)

proc newName(path: string) : string =
  # split the path into directory and name
  let (dir, name) = splitPath(path)
  return dir / fmt"{PREFIX}{name}{POSTFIX}"

proc test_create_and_delete_single_file() =
  # create a single file first
  info("\n\n=== Testing a single file creation and delete.")
  let content = "abcdefghij"
  let fname = "a.txt"
  let newname = newName(fname)
  writeFile(fname, content)
  info(fmt" Creating file {fname}")
  os.sleep(10)

  info(fmt" Deleting file {fname}")
  removeFile(fname)
  os.sleep(100)

  assert not fileExists(fname)  # must be deleted.
  # This should create newname
  assert fileExists(newname), fmt"{fname} is deleted. Expecting {newname} to exists."
  info("   test passed.")


proc test_create_and_delete_directory() =
  info("\n=== Testing a directory delete and backup.")
  let content : string = "abcdefghij"
  let dirname = "mytestdir"
  if(not existsOrCreateDir(dirname)):
    error(fmt"{dirname} does not exists.")

  var filesCreated : seq[string]
  for i in 1..4:
    let name : string = dirname / fmt"{i}.txt"
    info(fmt"Creating {name} ...")
    writeFile(name, content)
    filesCreated.add(name)
  os.sleep(100)  # sleep for a while

  # assert the new filestructure.
  var failed = false
  for f in filesCreated:
    removeFile(f)
    os.sleep(1)
    let ff = newName(f)
    assert fileExists(ff), fmt"{ff} must exists."
    assert not fileExists(f), fmt"{f} must not exists."

    # remove renamed file must fail.
    try:
      removeFile(ff)
      failed = true
    except OSError:
      info("Success. File could not be deleted.")
      failed = false

    if failed:
      raise newException(IOError, "When minifilter is running, {ff} should not be deleted.")

  info("All files are renamed successfully.")

  # now delete the directory.
  os.sleep(10)
  assert dirExists(dirname), fmt"{dirname} does not exists."
  try:
    removeDir(dirname, checkDir=true)
    failed = true
  except:
    info(fmt"Success. We should not be able to delete the directory.")
    failed = false

  if failed:
    raise newException(IOError, "When minifilter is running, {dirname} should not be deleted.")



when isMainModule:
  test_create_and_delete_single_file()
  test_create_and_delete_directory()
