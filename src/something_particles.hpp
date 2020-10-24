#ifndef SOMETHING_PARTICLES_HPP_
#define SOMETHING_PARTICLES_HPP_

const size_t PARTICLES_CAPACITY = 2048;

struct Particles
{
    enum State
    {
        DISABLED = 0,
        EMITTING
    };

    State state;
    Vec2f positions[PARTICLES_CAPACITY];
    Vec2f velocities[PARTICLES_CAPACITY];
    float lifetimes[PARTICLES_CAPACITY];
    float sizes[PARTICLES_CAPACITY];
    RGBA colors[PARTICLES_CAPACITY];
    float cooldown;
    HSLA current_color;
    Vec2f source;

    size_t begin;
    size_t count;

    void render(SDL_Renderer *renderer, Camera camera) const;
    void render_dark(SDL_Renderer *renderer, Camera camera) const;
    void update(float dt, Tile_Grid *grid);
    void update_dark(float dt);
    void push(float impact);
    void push_sparkle(float impact);
    void push_dark();
    void pop();
};

#endif  // SOMETHING_PARTICLES_HPP_
