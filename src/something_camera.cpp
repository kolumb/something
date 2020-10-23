const float SCREEN_WIDTH = 1920.0f;
const float SCREEN_HEIGHT = 1080.0f;

struct Camera
{
    Vec2f pos;
    Vec2f vel;
    float shake;

    Vec2f to_screen(Vec2f world_pos)
    {
        return world_pos - (pos - vec2((float) SCREEN_WIDTH, (float) SCREEN_HEIGHT) * 0.5f);
    }

    Rectf to_screen(Rectf world_rect)
    {
        return world_rect - (pos - vec2((float) SCREEN_WIDTH, (float) SCREEN_HEIGHT) * 0.5f);
    }

    Vec2f to_world(Vec2f screen_pos)
    {
        return screen_pos + (pos - vec2((float) SCREEN_WIDTH, (float) SCREEN_HEIGHT) * 0.5f);
    }

    void update(float delta_time)
    {
        if (shake > 0.001f) {
            float angle = rand_float_range(0, PI * 2.0f);
            auto shake_vec = polar(shake, angle);
            shake *= CAMERA_SHAKE_FADE_OUT;
            pos += vel * delta_time + shake_vec;
        } else {
            pos += vel * delta_time;
        }
    }
};

