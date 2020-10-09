env SOMETHIN_RELEASE=1 em++ src/something.cpp -Wcast-align -Wover-aligned -fno-inline -fno-inline-functions --profiling \
    -ISDL -std=c++17 -s WASM=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 \
    -s TOTAL_MEMORY=78643200 -s ALLOW_MEMORY_GROWTH=1 -s SAFE_HEAP=1 -s ABORTING_MALLOC=1 -s WARN_UNALIGNED=1 \
    -s DEMANGLE_SUPPORT=1 -s DETERMINISTIC=1 -s LLD_REPORT_UNDEFINED -s ASSERTIONS=1 \
    -o index.js --preload-file assets --use-preload-plugins --profiling-funcs -g
