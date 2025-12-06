# Backup on Delete

This repository contains two major components:

- `minifilter` directory contains a minifilter that intercepts the delete file
  event and instead of deleting the file, renames the file, hide it and send the new path to 
  a user-space client to create a backup.
- `app` directory has the user-space client that talks to minifilter. Upon receiving 
  the file path, it backs it up onto an S3 bucket and then asks the minifilter to 
  delete it. 
- When minifilter receives the delete request from the user-space app, it let the kernel delete
  the file.

In addition to this, we also provide `inf` file and nsis script to create the installer. The
build flow also self-sign the binary.


## Building 

- Install WDK and other related MS bloatware. 
- Install cmake 
- Install `boost` if you are building `app` as well.


### Building the driver

- Go to `minifilter` directory and,

```
mkdir _build
cd _build
cmake ..
cmake --build .
```

- To build `app`, the same steps in `app` directory.

