#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cerrno>
#include <cmath>
#include <cctype>
#include <cstring>

#include <SDL2/SDL.h>
#include "./aids.hpp"

using namespace aids;

const size_t CONFIG_VAR_CAPACITY = 1024;
String_View names[CONFIG_VAR_CAPACITY];
String_View types[CONFIG_VAR_CAPACITY];
size_t config_count = 0;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        println(stderr, "Usage: ./config_typer <config.var>");
        exit(1);
    }

    auto input = read_file_as_string_view(argv[1]);
    if (!input.has_value) {
        println(stderr, "Could not read file `", argv[1], "`");
        abort();
    }

    while (input.unwrap.count > 0) {
        // VAR_NAME : type = value
        auto line = input.unwrap.chop_by_delim('\n').trim();
        if (line.count == 0)   continue; // skip empty lines
        if (*line.data == '#') continue; // skip commentsa

        names[config_count] = line.chop_by_delim(':').trim();
        types[config_count] = line.chop_by_delim('=').trim();
        config_count += 1;
    }

    println(stdout, "// Generated by `", __FILE__, "` from `", argv[1], "`");
    println(stdout, "#define CONFIG_VAR_CAPACITY ", config_count);
    for (size_t i = 0; i < config_count; ++i) {
        println(stdout, "#define ", names[i], " config_values[", i, "].", types[i], "_value");
    }

    println(stdout);

    println(stdout,     "ssize_t config_index_by_name(String_View name) {");
    for (size_t i = 0; i < config_count; ++i) {
        println(stdout, "    if (name == \"", names[i], "\"_sv) return ", i, ";");
    }
    println(stdout,     "    return -1;");
    println(stdout,     "}");

    println(stdout);

    println(stdout,     "String_View config_names[CONFIG_VAR_CAPACITY] {");
    for (size_t i = 0; i < config_count; ++i) {
        println(stdout, "    \"", names[i], "\"_sv,");
    }
    println(stdout,     "};");

    println(stdout);

    println(stdout,     "Config_Type config_types[CONFIG_VAR_CAPACITY] {");
    for (size_t i = 0; i < config_count; ++i) {
        println(stdout, "    CONFIG_TYPE_", Caps { types[i] }, ",");
    }
    println(stdout,     "};");

    return 0;
}
