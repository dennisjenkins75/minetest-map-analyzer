# minetest-map-analyzer

C++ library and tool for extracting data from minetest maps.

## Building

There are two main ways to compile the binaries for this project:

1. Directly on your host machine.  You will likely need to install some
   missing developer packages (means vary by Linux distro).

   ```
   $ git clone --recursive git@github.com:dennisjenkins75/minetest-map-analyzer.git
   $ cd minetest-map-analyzer
   $ premake5 --os=linux gmake
   $ make -j20 config=release
   $ ./bin/Release/map_analyzer --help
   ```

1. Via docker, in a hermetic Ubuntu environment.  However, if your host
   environment is not Ubuntu Jammy (22.04), then the resulting binaries
   most likely won't run (I suppose I should statically link them...)

   ```
   $ git clone --recursive git@github.com:dennisjenkins75/minetest-map-analyzer.git
   $ cd minetest-map-analyzer
   $ ./build/docker-build.sh
   $ ./bin/Release/map_analyzer --help
   ```

## Usage

Read from `map.sqlite`, write to `output.sqlite`, use 28 threads, only spawn
new work when load average is below 20.

```
$ premake5 --os=linux gmake

$ make config=release

$ ./bin/Release/map_analyzer --map ./map.sqlite --out ./output.sqlite \
  -t 28 -l 20

$ sqlite3 -column -header output.sqlite < reports/minegeld-by-player.sql
```
