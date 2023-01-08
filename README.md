# minetest-map-analyzer

C++ library and tool for extracting data from minetest maps.

## Usage

Read from `map.sqlite`, write to `data.sqlite`, use 28 threads, only spawn
new work when load average is below 20.

```
$ premake5 --os=linux gmake

$ make -j20 config=release

$ ./bin/Release/map_analyzer --map ./map.sqlite --data ./output.sqlite \
  -t 28 -l 20
```
