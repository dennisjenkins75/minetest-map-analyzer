function set_cpp_dialect()
    cppdialect "C++2a"
end

function include_gtest()
    includedirs {
        "vendor/googletest/googletest/include",
        "vendor/googletest/googlemock/include",
    }

    links { "lib_gtest" }
end

function include_spdlog()
    includedirs {
        "vendor/spdlog/include",
    }

    -- TODO: Use the header-only libfmt?
    -- https://github.com/fmtlib/fmt/issues/2157
    links { "lib_spdlog", "fmt" }
end

-- TODO: Fix problem with linking our vendor/sqlite...
-- Using system sqlite3 for now.
function include_sqlite()
    includedirs {
--        "vendor/sqlite",
    }

--    links { "lib_sqlite" }
    links { "sqlite3" }
end

function cpp_library()
    kind "StaticLib"
    language "C++"
    set_cpp_dialect()
    systemversion "latest"

    includedirs {
        ".",
    }
end

function hide_project_makefile()
    -- Hide the per-project Makefiles.
    location (".premake")

    -- Keep "bin/" at the top level.
    filter { "configurations:Debug" }
        targetdir "bin/Debug"

    filter { "configurations:Release" }
        targetdir "bin/Release"

    -- Reset filter before exiting, or the 'Release' filter will remain active.
    filter {}
end

workspace "MinetestMapSearch"
    configurations { "Debug", "Release" }

    filter { "configurations:Debug" }
        optimize "Off"
        symbols "On"

    filter { "configurations:Release" }
        optimize "On"
        symbols "On"

project "lib_gtest"
    hide_project_makefile()
    cpp_library()
    includedirs {
        "vendor/googletest/googletest/include",
        "vendor/googletest/googletest",
        "vendor/googletest/googlemock/include",
        "vendor/googletest/googlemock"
    }
    files {
        "vendor/googletest/googletest/src/gtest-all.cc",
        "vendor/googletest/googlemock/src/gmock_main.cc",
        "vendor/googletest/googlemock/src/gmock-all.cc"
    }


project "lib_spdlog"
    hide_project_makefile()
    cpp_library()
    includedirs {
        "vendor/spdlog/include",
    }
    files {
        "vendor/spdlog/src/*.cpp",
    }
    defines {
        "SPDLOG_COMPILED_LIB",
    }

project "lib_sqlite"
    hide_project_makefile()
    kind "StaticLib"
    language "C"
    systemversion "latest"
    includedirs {
        "vendor/sqlite",
    }
    files {
        "vendor/sqlite/sqlite3.c",
    }
    buildoptions {
        "-DSQLITE_OMIT_LOAD_EXTENSION",
    }

project "map_reader_lib"
    hide_project_makefile()
    cpp_library()
    files {
        "src/lib/map_reader/**.cc",
    }
    removefiles {
        "src/lib/map_reader/**_test.cc",
    }
    filter {"action:gmake or action:gmake2"}
        enablewarnings{"all"}

project "database_lib"
    hide_project_makefile()
    cpp_library()
    files {
        "src/lib/database/**.cc",
    }
    removefiles {
        "src/lib/database/**_test.cc",
    }
    filter {"action:gmake or action:gmake2"}
        enablewarnings{"all"}



project "unit_tests"
    hide_project_makefile()
    kind "ConsoleApp"
    language "C++"
    set_cpp_dialect()
    includedirs {
        ".",
    }
    files {
       "src/**_test.cc",
    }
    systemversion "latest"
    links { "database_lib", "map_reader_lib", "absl_strings" }
    include_gtest()
    include_spdlog()
    include_sqlite()

    filter { "system:linux" }
        links { "pthread", "z" }

    filter { "action:gmake or action:gmake2" }
        disablewarnings { "sign-compare" }
        enablewarnings { "all" }

project "map_analyzer"
    hide_project_makefile()
    kind "ConsoleApp"
    language "C++"
    set_cpp_dialect()
    includedirs {
        ".",
    }
    files {
        "src/app/**.cc"
    }
    removefiles {
        "src/app/**_test.cc",
    }
    include_spdlog()
    include_sqlite()
    systemversion "latest"
    links { "database_lib", "map_reader_lib", "absl_strings" }

    filter { "system:linux" }
        links { "pthread", "z" }

    filter { "action:gmake or action:gmake2" }
        disablewarnings { "sign-compare" }
        enablewarnings { "all" }