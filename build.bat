set SOMETHING_RELEASE=1 && em++ -O3 src/something.cpp -ISDL -std=c++17 -s WASM=1 -s USE_SDL=2 -s USE_SDL_IMAGE=2 ^
    -s TOTAL_MEMORY=78643200 -s ALLOW_MEMORY_GROWTH=1 -s EXIT_RUNTIME=1 ^
    -o index.js --preload-file assets --use-preload-plugins