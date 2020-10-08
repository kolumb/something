#include <SDL_image.h>
#include "./something_texture.hpp"

void load_textures(SDL_Renderer *renderer)
{
    for (size_t i = 0; i < TEXTURE_COUNT; ++i) {
        if (textures[i] == nullptr) {
            surfaces[i] = load_png_file_as_surface(texture_files[i]);
            textures[i] = sec(SDL_CreateTextureFromSurface(renderer, surfaces[i]));

            surface_masks[i] = load_png_file_as_surface(texture_files[i]);

            sec(SDL_LockSurface(surface_masks[i]));
            assert(surface_masks[i]->format->format == SDL_PIXELFORMAT_RGBA32);
            for (int row = 0; row < surface_masks[i]->h; ++row) {
                uint32_t *pixel_row = (uint32_t*) ((uint8_t *) surface_masks[i]->pixels + row * surface_masks[i]->pitch);
                for (int col = 0; col < surface_masks[i]->w; ++col) {
                    pixel_row[col] = pixel_row[col] | 0x00FFFFFF;
                }
            }
            SDL_UnlockSurface(surface_masks[i]);

            texture_masks[i] = sec(SDL_CreateTextureFromSurface(renderer, surface_masks[i]));
        }
    }
}

Texture_Index texture_index_by_name(String_View filename)
{
    for (size_t i = 0; i < TEXTURE_COUNT; ++i) {
        if (filename == cstr_as_string_view(texture_files[i])) {
            return {i};
        }
    }

#ifndef SOMETHING_RELEASE
    println(stderr,
            "[ERROR] Unknown texture file `", filename, "`. ",
            "You may want to add it to the `textures` array.");
    abort();
#endif

    return {0};
}

SDL_Texture *load_texture_from_bmp_file(SDL_Renderer *renderer,
                                        const char *image_filepath,
                                        SDL_Color color_key)
{
    SDL_Surface *image_surface = sec(SDL_LoadBMP(image_filepath));

    sec(SDL_SetColorKey(
            image_surface,
            SDL_TRUE,
            SDL_MapRGB(
                image_surface->format,
                color_key.r,
                color_key.g,
                color_key.b)));

    SDL_Texture *image_texture = sec(SDL_CreateTextureFromSurface(renderer, image_surface));
    SDL_FreeSurface(image_surface);
    return image_texture;
}

SDL_Surface *load_png_file_as_surface(const char *image_filename)
{
    return sec(IMG_Load(image_filename));
}
