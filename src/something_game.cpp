#include "something_game.hpp"

const char *projectile_state_as_cstr(Projectile_State state)
{
    switch (state) {
    case Projectile_State::Ded: return "Ded";
    case Projectile_State::Active: return "Active";
    case Projectile_State::Poof: return "Poof";
    }

    assert(0 && "Incorrect Projectile_State");
    return "";
}

template <typename ... Types>
void displayf(SDL_Renderer *renderer,
              Bitmap_Font *font,
              RGBA color,
              RGBA shadow_color,
              Vec2f p,
              Types... args)
{
    char text[256];
    String_Buffer sbuffer = {256, text, 0};
    sprintln(&sbuffer, args...);

    auto font_size = vec2(FONT_DEBUG_SIZE, FONT_DEBUG_SIZE);
    font->render(renderer, p - vec2(2.0f, 2.0f), font_size, shadow_color, text);
    font->render(renderer, p, font_size, color, text);
}

void Projectile::kill()
{
    if (state == Projectile_State::Active) {
        state = Projectile_State::Poof;
        poof_animat.reset();
    }
}

void Game::handle_event(SDL_Event *event)
{
    // GLOBAL KEYBINDINGS //////
    switch (event->type) {
    case SDL_QUIT: {
        quit = true;
    } break;

    case SDL_KEYDOWN: {
        switch (event->key.keysym.sym) {
        case SDLK_BACKQUOTE: {
            console.toggle();
        } break;


#ifndef SOMETHING_RELEASE
        case SDLK_F2: {
            fps_debug = !fps_debug;
        } break;

        case SDLK_F3: {
            bfs_debug = !bfs_debug;
        } break;

        case SDLK_F5: {
            command_reload(this, ""_sv);
        } break;
#endif  // SOMETHING_RELEASE
        }
    } break;

    case SDL_MOUSEMOTION: {
        mouse_position =
            camera.to_world(vec_cast<float>(vec2(event->motion.x, event->motion.y)));
        original_mouse_position = vec2(event->motion.x, event->motion.y);
        collision_probe = mouse_position;

        if (debug) {
            debug_toolbar.handle_mouse_hover(
                vec_cast<float>(vec2(event->motion.x, event->motion.y)));
        }

        grid.resolve_point_collision(&collision_probe);

        Vec2i tile = vec_cast<int>(mouse_position / TILE_SIZE);
        switch (draw_state) {
        case Debug_Draw_State::Create: {
            grid.set_tile(tile, draw_tile);
        } break;

        case Debug_Draw_State::Delete: {
            grid.set_tile(tile, TILE_EMPTY);
        } break;

        case Debug_Draw_State::Idle:
        default: {}
        }

        if (entities[PLAYER_ENTITY_INDEX].current_weapon == Weapon::Dirt_Block && holding_down_mouse) {
            entity_shoot({PLAYER_ENTITY_INDEX});
        }
    } break;

    case SDL_MOUSEBUTTONDOWN: {
        switch (event->button.button) {
        case SDL_BUTTON_RIGHT: {
            if (debug) {
                tracking_projectile =
                    projectile_at_position(mouse_position);

                if (!tracking_projectile.has_value) {
                    switch (debug_toolbar.active_button) {
                    case DEBUG_TOOLBAR_TILES:
                    case DEBUG_TOOLBAR_DESTROYABLE: {
                        if (debug_toolbar.active_button == DEBUG_TOOLBAR_TILES) {
                            draw_tile = TILE_WALL;
                        } else {
                            draw_tile = TILE_DESTROYABLE_0;
                        }

                        Vec2i tile = vec_cast<int>(mouse_position / TILE_SIZE);

                        if (grid.get_tile(tile) == TILE_EMPTY) {
                            draw_state = Debug_Draw_State::Create;
                            grid.set_tile(tile, draw_tile);
                        } else {
                            draw_state = Debug_Draw_State::Delete;
                            grid.set_tile(tile, TILE_EMPTY);
                        }
                    } break;

                    case DEBUG_TOOLBAR_HEALS: {
                        spawn_health_at_mouse();
                    } break;

                    case DEBUG_TOOLBAR_ENEMIES: {
                        spawn_enemy_at(mouse_position);
                    } break;

                    default: {}
                    }
                }
            } else {
                time_bomb = mouse_position;
                Vec2i mouse_tile = grid.abs_to_tile_coord(mouse_position);
                const int r = EXPLOSION_RADIUS_IN_TILES;
                size_t search_index = 0;
                for (int y = -r; y <= r; ++y) {
                    for (int x = -r; x <= r; ++x) {
                        auto probe = vec2(x, y);
                        // NOTE: Subtraction of r is needed for rounded corners (2 <= r <= 6).
                        if(sqr_len(probe) < r*r-r) {
                            Vec2i tile_coord = mouse_tile + probe;
                            Tile tile = grid.get_tile(tile_coord);
                            if(tile == TILE_EMPTY) continue;
                            grid.set_tile(tile_coord, TILE_EMPTY);

                            while (search_index < EXPLODED_TILES_COUNT) {
                                if (exploded_tiles[search_index].state == Exploded_Tile_State::Ded) {
                                    Vec2f tile_pos = vec_cast<float>(tile_coord) * TILE_SIZE;
                                    Vec2f center_of_tile = tile_pos + vec2(0.5f * TILE_SIZE, 0.5f * TILE_SIZE);

                                    exploded_tiles[search_index] = make_exploded_tile(tile_pos, tile, mouse_position);

                                    Vec2f vec_from_epicenter = center_of_tile - mouse_position;
                                    float dist_from_epicenter_sqr = sqr_len(vec_from_epicenter);
                                    float power = EXPLOSION_RADIUS_SQR / dist_from_epicenter_sqr;
                                    Vec2f impulse = vec_from_epicenter * clamp(power, 0.0f, MAX_EXPLOSION_IMPULSE);
                                    exploded_tiles[search_index].vel = impulse * EXPLOSION_POWER;
                                    break;
                                }
                                search_index++;
                            }
                        }
                    }
                }
            }
        } break;

        case SDL_BUTTON_LEFT: {
            if (!debug_toolbar.handle_click_at({(float) event->button.x, (float) event->button.y})) {
                entity_shoot({PLAYER_ENTITY_INDEX});
            }

            holding_down_mouse = true;
        } break;
        }
    } break;

    case SDL_MOUSEBUTTONUP: {
        switch (event->button.button) {
        case SDL_BUTTON_RIGHT: {
            draw_state = Debug_Draw_State::Idle;
        } break;

        case SDL_BUTTON_LEFT: {
            holding_down_mouse = false;
        } break;
        }
    } break;
    }


    if (console.enabled) {
        console.handle_event(event, this);
    } else {
        switch (event->type) {
        case SDL_KEYDOWN: {
            switch (event->key.keysym.sym) {
            case SDLK_1: {
                entities[PLAYER_ENTITY_INDEX].current_weapon = Weapon::Gun;
            } break;

            case SDLK_2: {
                entities[PLAYER_ENTITY_INDEX].current_weapon = Weapon::Dirt_Block;
            } break;

            case SDLK_SPACE: {
                if (!event->key.repeat) {
                    entity_jump({PLAYER_ENTITY_INDEX});
                }
            } break;

            case SDLK_q: {
                debug = !debug;
            } break;

            case SDLK_z: {
                step_debug = !step_debug;
            } break;

            case SDLK_r: {
                reset_entities();
            } break;
            }
        } break;
        }
    }
}

void Game::update(float dt)
{
    // Update Player's gun direction //////////////////////////////
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    entities[PLAYER_ENTITY_INDEX].point_gun_at(mouse_position);

    // Enemy AI //////////////////////////////
    auto &player = entities[PLAYER_ENTITY_INDEX];
    Recti *lock = NULL;
    for (size_t i = 0; i < camera_locks_count; ++i) {
        Rectf lock_abs = rect_cast<float>(camera_locks[i]) * TILE_SIZE;
        if (rect_contains_vec2(lock_abs, player.pos)) {
            lock = &camera_locks[i];
        }
    }

    auto player_tile = grid.abs_to_tile_coord(player.pos);
    if (lock) {
        grid.bfs_to_tile(player_tile, lock);
    }

    if (!debug && lock) {
        Rectf lock_abs = rect_cast<float>(*lock) * TILE_SIZE;
        for (size_t i = ENEMY_ENTITY_INDEX_OFFSET; i < ENTITIES_COUNT; ++i) {
            auto &enemy =  entities[i];
            if (enemy.state == Entity_State::Alive) {
                if (rect_contains_vec2(lock_abs, enemy.pos)) {
                    if (grid.a_sees_b(enemy.pos, player.pos)) {

                        if(!enemy.knows_about_player) {
                            enemy.knows_about_player = true;
                            for (size_t j = ENEMY_ENTITY_INDEX_OFFSET; j < ENTITIES_COUNT; ++j) {
                                auto &enemy_in_room = entities[j];
                                if (enemy_in_room.state == Entity_State::Alive) {
                                    if (rect_contains_vec2(lock_abs, enemy_in_room.pos)) {
                                        enemy_in_room.knows_about_player = true;
                                    }
                                }
                            }
                        }
                        enemy.stop();
                        enemy.point_gun_at(player.pos);
                        entity_shoot({i});
                    } else {
                        auto enemy_tile = grid.abs_to_tile_coord(enemy.pos);
                        auto current_bfs_value = grid.bfs_trace[enemy_tile.y - lock->y][enemy_tile.x - lock->x];

                        if(!enemy.knows_about_player) {
                            if(current_bfs_value > DETECTION_RANGE) {
                                continue;
                            } else {
                                for (size_t j = ENEMY_ENTITY_INDEX_OFFSET; j < ENTITIES_COUNT; ++j) {
                                    auto &enemy_in_room = entities[j];
                                    if (enemy_in_room.state == Entity_State::Alive) {
                                        if (rect_contains_vec2(lock_abs, enemy_in_room.pos)) {
                                            enemy_in_room.knows_about_player = true;
                                        }
                                    }
                                }
                            }
                        }

                        auto next = grid.next_in_bfs(enemy_tile, lock);
                        if (next.has_value) {
                            auto d = next.unwrap - enemy_tile;

                            if (d.y < 0) {
                                enemy.jump();
                            }
                            if (d.x > 0) {
                                enemy.move(Entity::Right);
                            }
                            if (d.x < 0) {
                                enemy.move(Entity::Left);
                            }
                            if (d.x == 0) {
                                enemy.stop();
                            }
                        } else {
                            enemy.stop();
                        }
                    }
                }
            }
        }
    }

    // Update All Exploded Tiles //////////////////////////////
    for (size_t i = 0; i < EXPLODED_TILES_COUNT; ++i) {
        exploded_tiles[i].update(apply_time_bomb(dt, exploded_tiles[i].pos));
        exploded_tile_check_for_collision({i});
    }

    // Update All Entities //////////////////////////////
    for (size_t i = 0; i < ENTITIES_COUNT; ++i) {
        entities[i].update(apply_time_bomb(dt, entities[i].pos), &mixer, &grid);
        entity_resolve_collision({i});
        entities[i].has_jumped = false;
    }

    // Update All Projectiles //////////////////////////////
    update_projectiles(dt);

    // Update Items //////////////////////////////
    for (size_t i = 0; i < ITEMS_COUNT; ++i) {
        items[i].update(apply_time_bomb(dt, items[i].pos));
    }

    // Entities/Projectiles interaction //////////////////////////////
    for (size_t index = 0; index < PROJECTILES_COUNT; ++index) {
        auto projectile = projectiles + index;
        if (projectile->state != Projectile_State::Active) continue;

        for (size_t entity_index = 0;
             entity_index < ENTITIES_COUNT;
             ++entity_index)
        {
            auto entity = entities + entity_index;

            if (entity->state != Entity_State::Alive) continue;
            if (entity_index == projectile->shooter.unwrap) continue;

            if (rect_contains_vec2(entity->hitbox_world(), projectile->pos)) {
                projectile->kill();
                entity->lives -= ENTITY_PROJECTILE_DAMAGE;

                mixer.play_sample(damage_enemy_sample);
                if (entity->lives <= 0) {
                    entity->kill();
                    mixer.play_sample(kill_enemy_sample);
                } else {
                    entity->vel += normalize(projectile->vel) * ENTITY_PROJECTILE_KNOCKBACK;
                    entity->flash(ENTITY_DAMAGE_FLASH_COLOR);
                }
            }
        }
    }

    // Entities/Items interaction
    for (size_t index = 0; index < ITEMS_COUNT; ++index) {
        auto item = items + index;
        if (item->type == ITEM_HEALTH) {
            for (size_t entity_index = 0;
                 entity_index < ENTITIES_COUNT;
                 ++entity_index)
            {
                auto entity = entities + entity_index;

                if (entity->state == Entity_State::Alive) {
                    if (rects_overlap(entity->hitbox_world(), item->hitbox_world())) {
                        entity->lives = min(entity->lives + ITEM_HEALTH_POINTS, ENTITY_MAX_LIVES);
                        entity->flash(ENTITY_HEAL_FLASH_COLOR);
                        mixer.play_sample(item->sound);
                        item->type = ITEM_NONE;
                        break;
                    }
                }
            }
        }
    }

    // Player Movement //////////////////////////////
    if (!console.enabled) {
        if (keyboard[SDL_SCANCODE_D]) {
            entities[PLAYER_ENTITY_INDEX].move(Entity::Right);
        } else if (keyboard[SDL_SCANCODE_A]) {
            entities[PLAYER_ENTITY_INDEX].move(Entity::Left);
        } else {
            entities[PLAYER_ENTITY_INDEX].stop();
        }
    }

    // Camera "Physics" //////////////////////////////
    const auto player_pos = entities[PLAYER_ENTITY_INDEX].pos;
    camera.vel = (player_pos - camera.pos) * PLAYER_CAMERA_FORCE;

    for (size_t i = 0; i < camera_locks_count; ++i) {
        Rectf lock_abs = rect_cast<float>(camera_locks[i]) * TILE_SIZE;
        if (rect_contains_vec2(lock_abs, player_pos)) {
            camera.vel += (rect_center(lock_abs) - camera.pos) * CENTER_CAMERA_FORCE;
        }
    }

    camera.update(dt);

    // Popup //////////////////////////////
    popup.update(dt);

    // Console //////////////////////////////
    console.update(dt);
}

void Game::render(SDL_Renderer *renderer)
{
    Recti *lock = NULL;
    for (size_t i = 0; i < camera_locks_count; ++i) {
        Rectf lock_abs = rect_cast<float>(camera_locks[i]) * TILE_SIZE;
        if (rect_contains_vec2(lock_abs, entities[PLAYER_ENTITY_INDEX].pos)) {
            lock = &camera_locks[i];
        }
    }

    background.render(renderer, camera);

    if (bfs_debug && lock) {
        grid.render_debug_bfs_overlay(
            renderer,
            &camera,
            lock);
    }

    render_time_bomb_radius(renderer);

    grid.render(renderer, camera, lock);

    for (size_t i = 0; i < ENTITIES_COUNT; ++i) {
        // TODO(#106): display health bar differently for enemies in a different room
        entities[i].render(renderer, camera);
    }

    switch (entities[PLAYER_ENTITY_INDEX].current_weapon) {
    case Weapon::Dirt_Block: {
        bool can_place = false;
        auto target_tile = where_entity_can_place_block({PLAYER_ENTITY_INDEX}, &can_place);

        can_place = can_place && entities[PLAYER_ENTITY_INDEX].dirt_blocks_count > 0;

        tile_defs[TILE_DESTROYABLE_0].top_texture.render(
            renderer,
            rect(camera.to_screen(vec2((float) target_tile.x, (float) target_tile.y) * TILE_SIZE), TILE_SIZE, TILE_SIZE),
            SDL_FLIP_NONE,
            can_place ? CAN_PLACE_DIRT_BLOCK_COLOR : CANNOT_PLACE_DIRT_BLOCK_COLOR);
    } break;

    case Weapon::Gun: {
    } break;
    }

    render_projectiles(renderer, camera);

    for (size_t i = 0; i < ITEMS_COUNT; ++i) {
        if (items[i].type != ITEM_NONE) {
            items[i].render(renderer, camera);
        }
    }

    for (size_t i = 0; i < EXPLODED_TILES_COUNT; ++i) {
        if (exploded_tiles[i].state != Exploded_Tile_State::Ded) {
            exploded_tiles[i].render(renderer, camera);
        }
    }
    
    if (fps_debug) {
        render_fps_overlay(renderer);
    }

    render_player_hud(renderer);

    popup.render(renderer);
    console.render(renderer, &debug_font);
}

void Game::entity_shoot(Entity_Index entity_index)
{
    assert(entity_index.unwrap < ENTITIES_COUNT);
    Entity *entity = &entities[entity_index.unwrap];

    if (entity->state == Entity_State::Alive) {
        switch (entity->current_weapon) {
        case Weapon::Gun: {
            if (entity->cooldown_weapon <= 0) {
                spawn_projectile(
                    entity->pos,
                    normalize(entity->gun_dir) * PROJECTILE_SPEED,
                    entity_index);
                entity->cooldown_weapon = ENTITY_COOLDOWN_WEAPON;

                mixer.play_sample(entity->shoot_sample);
            }
        } break;

        case Weapon::Dirt_Block: {
            // TODO(#222): dirt blocks must not be placed on the player
            bool can_place = false;
            auto target_tile = where_entity_can_place_block(entity_index, &can_place);
            if (can_place && entity->dirt_blocks_count > 0) {
                grid.set_tile(target_tile, TILE_DESTROYABLE_0);
                entity->dirt_blocks_count -= 1;
            }
        } break;
        }
    }
}

bool Game::does_tile_contain_entity(Vec2i tile_coord)
{
    Rectf tile_rect = grid.rect_of_tile(tile_coord);

    for (size_t i = 0; i < ENTITIES_COUNT; ++i) {
        if (entities[i].state == Entity_State::Alive && rects_overlap(tile_rect, entities[i].hitbox_world())) {
            return true;
        }
    }

    return false;
}

Vec2i Game::where_entity_can_place_block(Entity_Index index, bool *can_place)
{
    Entity *entity = &entities[index.unwrap];
    const auto allowed_length = min(length(entity->gun_dir), DIRT_BLOCK_PLACEMENT_PROXIMITY);
    const auto allowed_target = entity->pos + allowed_length *normalize(entity->gun_dir);
    const auto target_tile = grid.abs_to_tile_coord(allowed_target);

    if (can_place) {
        *can_place = grid.get_tile(target_tile) == TILE_EMPTY &&
            grid.a_sees_b(entity->pos, grid.abs_center_of_tile(target_tile)) &&
            !does_tile_contain_entity(target_tile);
    }

    return target_tile;
}


void Game::entity_jump(Entity_Index entity_index)
{
    assert(entity_index.unwrap < ENTITIES_COUNT);
    entities[entity_index.unwrap].jump();
}

void Game::reset_entities()
{
    static_assert(ROOM_ROW_COUNT > 0);
    entities[PLAYER_ENTITY_INDEX] = player_entity(vec2(200.0f, 200.0f));
}

void Game::entity_resolve_collision(Entity_Index entity_index)
{
    assert(entity_index.unwrap < ENTITIES_COUNT);
    Entity *entity = &entities[entity_index.unwrap];

    if (entity->state == Entity_State::Alive) {
        const float step_x = entity->hitbox_local.w / (float) ENTITY_MESH_COLS;
        const float step_y = entity->hitbox_local.h / (float) ENTITY_MESH_ROWS;

        for (int rows = 0; rows <= ENTITY_MESH_ROWS; ++rows) {
            for (int cols = 0; cols <= ENTITY_MESH_COLS; ++cols) {
                Vec2f t0 = entity->pos +
                    vec2(entity->hitbox_local.x, entity->hitbox_local.y) +
                    vec2(cols * step_x, rows * step_y);
                Vec2f t1 = t0;

                grid.resolve_point_collision(&t1);

                Vec2f d = t1 - t0;

                const int IMPACT_THRESHOLD = 5;
                if (abs(d.y) >= IMPACT_THRESHOLD && !entity->has_jumped) {
                    if (fabsf(entity->vel.y) > LANDING_PARTICLE_BURST_THRESHOLD) {
                        for (int i = 0; i < ENTITY_JUMP_PARTICLE_BURST; ++i) {
                            entity->particles.push(rand_float_range(PARTICLE_JUMP_VEL_LOW, fabsf(entity->vel.y) * 0.25f));
                        }
                    }

                    entity->vel.y = 0;
                }
                if (abs(d.x) >= IMPACT_THRESHOLD) entity->vel.x = 0;

                entity->pos += d;
            }
        }
    }
}

void Game::exploded_tile_check_for_collision(Exploded_Tile_Index exploded_tile_index)
{
    assert(exploded_tile_index.unwrap < EXPLODED_TILES_COUNT);
    Exploded_Tile *exploded_tile = &exploded_tiles[exploded_tile_index.unwrap];

    if (exploded_tile->state == Exploded_Tile_State::Alive) {
        Vec2i tile_coord = grid.abs_to_tile_coord(exploded_tile->pos);
        if(!grid.is_tile_empty_tile(tile_coord)
            || !grid.is_tile_empty_tile(tile_coord + vec2(1, 0))
            || !grid.is_tile_empty_tile(tile_coord + vec2(1, 1))
            || !grid.is_tile_empty_tile(tile_coord + vec2(0, 1))) {
            Vec2f exploded_tile_center = exploded_tile->pos + vec2(0.5f * TILE_SIZE, 0.5f * TILE_SIZE);
            if(sqr_dist(exploded_tile->epicenter, exploded_tile_center) > EXPLOSION_COOLDOWN_RADIUS) {
                grid.set_tile(grid.abs_to_tile_coord(exploded_tile_center), exploded_tile->tile);
            }
            exploded_tile->state = Exploded_Tile_State::Ded;
        }
    }
}

void Game::spawn_projectile(Vec2f pos, Vec2f vel, Entity_Index shooter)
{
    for (size_t i = 0; i < PROJECTILES_COUNT; ++i) {
        if (projectiles[i].state == Projectile_State::Ded) {
            projectiles[i].state = Projectile_State::Active;
            projectiles[i].pos = pos;
            projectiles[i].vel = vel;
            projectiles[i].shooter = shooter;
            projectiles[i].lifetime = PROJECTILE_LIFETIME;
            projectiles[i].active_animat = projectile_active_animat;
            projectiles[i].poof_animat = projectile_poof_animat;
            return;
        }
    }
}

void Game::render_debug_overlay(SDL_Renderer *renderer, size_t fps)
{
    sec(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255));

    const float COLLISION_PROBE_SIZE = 10.0f;
    const auto collision_probe_rect = rect(
        camera.to_screen(collision_probe - COLLISION_PROBE_SIZE),
        COLLISION_PROBE_SIZE * 2, COLLISION_PROBE_SIZE * 2);
    {
        auto rect = rectf_for_sdl(collision_probe_rect);
        sec(SDL_RenderFillRect(renderer, &rect));
    }

    const float PADDING = 10.0f;
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, PADDING),
             "FPS: ", fps);
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, 50 + PADDING),
             "Mouse Position: ",
             mouse_position.x, " ",
             mouse_position.y);
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, 2 * 50 + PADDING),
             "Collision Probe: ",
             collision_probe.x, " ",
             collision_probe.y);
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, 3 * 50 + PADDING),
             "Projectiles: ",
             count_alive_projectiles());
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, 4 * 50 + PADDING),
             "Player position: ",
             entities[PLAYER_ENTITY_INDEX].pos.x, " ",
             entities[PLAYER_ENTITY_INDEX].pos.y);
    displayf(renderer, &debug_font,
             FONT_DEBUG_COLOR,
             FONT_SHADOW_COLOR,
             vec2(PADDING, 5 * 50 + PADDING),
             "Player velocity: ",
             entities[PLAYER_ENTITY_INDEX].vel.x, " ",
             entities[PLAYER_ENTITY_INDEX].vel.y);

    if (tracking_projectile.has_value) {
        auto projectile = projectiles[tracking_projectile.unwrap.unwrap];
        const float SECOND_COLUMN_OFFSET = 700.0f;
        const RGBA TRACKING_DEBUG_COLOR = sdl_to_rgba({255, 255, 150, 255});
        displayf(renderer, &debug_font,
                 TRACKING_DEBUG_COLOR,
                 FONT_SHADOW_COLOR,
                 vec2(PADDING + SECOND_COLUMN_OFFSET, PADDING),
                 "State: ", projectile_state_as_cstr(projectile.state));
        displayf(renderer, &debug_font,
                 TRACKING_DEBUG_COLOR,
                 FONT_SHADOW_COLOR,
                 vec2(PADDING + SECOND_COLUMN_OFFSET, 50 + PADDING),
                 "Position: ",
                 projectile.pos.x, " ", projectile.pos.y);
        displayf(renderer, &debug_font,
                 TRACKING_DEBUG_COLOR,
                 FONT_SHADOW_COLOR,
                 vec2(PADDING + SECOND_COLUMN_OFFSET, 2 * 50 + PADDING),
                 "Velocity: ",
                 projectile.vel.x, " ", projectile.vel.y);
        displayf(renderer, &debug_font,
                 TRACKING_DEBUG_COLOR,
                 FONT_SHADOW_COLOR,
                 vec2(PADDING + SECOND_COLUMN_OFFSET, 3 * 50 + PADDING),
                 "Shooter Index: ",
                 projectile.shooter.unwrap);
    }

    for (size_t i = 0; i < ENTITIES_COUNT; ++i) {
        if (entities[i].state == Entity_State::Ded) continue;

        sec(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255));
        auto dstrect = rectf_for_sdl(camera.to_screen(entities[i].texbox_world()));
        sec(SDL_RenderDrawRect(renderer, &dstrect));

        sec(SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255));
        auto hitbox = rectf_for_sdl(camera.to_screen(entities[i].hitbox_world()));
        sec(SDL_RenderDrawRect(renderer, &hitbox));

        entities[i].render_debug(renderer, camera);
    }

    if (tracking_projectile.has_value) {
        sec(SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255));
        auto hitbox = rectf_for_sdl(
            camera.to_screen(hitbox_of_projectile(tracking_projectile.unwrap)));
        sec(SDL_RenderDrawRect(renderer, &hitbox));
    }

    auto projectile_index = projectile_at_position(mouse_position);
    if (projectile_index.has_value) {
        sec(SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255));
        auto hitbox = rectf_for_sdl(
            camera.to_screen(hitbox_of_projectile(projectile_index.unwrap)));
        sec(SDL_RenderDrawRect(renderer, &hitbox));
    } else {
        sec(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255));
        const Rectf tile_rect = {
            floorf(mouse_position.x / TILE_SIZE) * TILE_SIZE,
            floorf(mouse_position.y / TILE_SIZE) * TILE_SIZE,
            TILE_SIZE,
            TILE_SIZE
        };

        auto rect = rectf_for_sdl(camera.to_screen(tile_rect));
        sec(SDL_RenderDrawRect(renderer, &rect));
    }

    for (size_t i = 0; i < ITEMS_COUNT; ++i) {
        items[i].render_debug(renderer, camera);
    }

    debug_toolbar.render(renderer, debug_font);
}

void Game::render_fps_overlay(SDL_Renderer *renderer) {
    const float PADDING = 20.0f;
    const float SCALE = 5000.0f;
    const float BAR_WIDTH = 2.0f;
    for (size_t i = 0; i < FPS_BARS_COUNT; ++i) {
        size_t j = (frame_delays_begin + i) % FPS_BARS_COUNT;
        fill_rect(
            renderer,
            rect(
                vec2(SCREEN_WIDTH - PADDING - (float) (FPS_BARS_COUNT - j) * BAR_WIDTH,
                    SCREEN_HEIGHT - PADDING - frame_delays[j] * SCALE),
                BAR_WIDTH,
                frame_delays[j] * SCALE),
                {   clamp(       frame_delays[j] * 60.0f - 1.0f, 0.0f, 1.0f),
                    clamp(2.0f - frame_delays[j] * 60.0f       , 0.0f, 1.0f),
                    0, (float) i / (float) FPS_BARS_COUNT});
    }
}

int Game::count_alive_projectiles(void)
{
    int res = 0;
    for (size_t i = 0; i < PROJECTILES_COUNT; ++i) {
        if (projectiles[i].state != Projectile_State::Ded) ++res;
    }
    return res;
}

void Game::render_projectiles(SDL_Renderer *renderer, Camera camera)
{
    for (size_t i = 0; i < PROJECTILES_COUNT; ++i) {
        switch (projectiles[i].state) {
        case Projectile_State::Active: {
            projectiles[i].active_animat.render(
                renderer,
                camera.to_screen(projectiles[i].pos));
        } break;

        case Projectile_State::Poof: {
            projectiles[i].poof_animat.render(
                renderer,
                camera.to_screen(projectiles[i].pos));
        } break;

        case Projectile_State::Ded: {} break;
        }
    }
}

void Game::update_projectiles(float dt)
{
    for (size_t i = 0; i < PROJECTILES_COUNT; ++i) {
        switch (projectiles[i].state) {
        case Projectile_State::Active: {
            float new_dt = apply_time_bomb(dt, projectiles[i].pos);
            projectiles[i].active_animat.update(new_dt);
            projectiles[i].pos += projectiles[i].vel * new_dt;

            auto tile = grid.tile_at_abs(projectiles[i].pos);
            if (tile && tile_defs[*tile].is_collidable) {
                projectiles[i].kill();
                if (TILE_DESTROYABLE_0 <= *tile && *tile < TILE_DESTROYABLE_3) {
                    *tile += 1;
                } else if (*tile == TILE_DESTROYABLE_3) {
                    *tile = TILE_EMPTY;
                }
            }

            projectiles[i].lifetime -= new_dt;

            if (projectiles[i].lifetime <= 0.0f) {
                projectiles[i].kill();
            }
        } break;

        case Projectile_State::Poof: {
            projectiles[i].poof_animat.update(apply_time_bomb(dt, projectiles[i].pos));
            if (projectiles[i].poof_animat.frame_current ==
                (projectiles[i].poof_animat.frame_count - 1)) {
                projectiles[i].state = Projectile_State::Ded;
            }
        } break;

        case Projectile_State::Ded: {} break;
        }
    }
}

const float PROJECTILE_TRACKING_PADDING = 50.0f;

Rectf Game::hitbox_of_projectile(Projectile_Index index)
{
    assert(index.unwrap < PROJECTILES_COUNT);
    return Rectf {
        projectiles[index.unwrap].pos.x - PROJECTILE_TRACKING_PADDING * 0.5f,
            projectiles[index.unwrap].pos.y - PROJECTILE_TRACKING_PADDING * 0.5f,
            PROJECTILE_TRACKING_PADDING,
            PROJECTILE_TRACKING_PADDING
            };
}

Maybe<Projectile_Index> Game::projectile_at_position(Vec2f position)
{
    for (size_t i = 0; i < PROJECTILES_COUNT; ++i) {
        if (projectiles[i].state == Projectile_State::Ded) continue;

        Rectf hitbox = hitbox_of_projectile({i});
        if (rect_contains_vec2(hitbox, position)) {
            return {true, {i}};
        }
    }

    return {};
}

void Game::spawn_health_at_mouse()
{
    for (size_t i = 0; i < ITEMS_COUNT; ++i) {
        if (items[i].type == ITEM_NONE) {
            items[i] = make_health_item(mouse_position);
            break;
        }
    }
}

void Game::add_camera_lock(Recti rect)
{
    assert(camera_locks_count < CAMERA_LOCKS_CAPACITY);
    camera_locks[camera_locks_count++] = rect;
}

void Game::spawn_enemy_at(Vec2f pos)
{
    for (size_t i = PLAYER_ENTITY_INDEX + ENEMY_ENTITY_INDEX_OFFSET; i < ENTITIES_COUNT; ++i) {
        if (entities[i].state == Entity_State::Ded) {
            entities[i] = enemy_entity(pos);
            break;
        }
    }
}

int Game::get_rooms_count(void)
{
    int result = 0;
    DIR *rooms_dir = opendir("./assets/rooms/");
    if (rooms_dir == NULL) {
        println(stderr, "Can't open asset folder: ./assets/rooms/");
        abort();
    }

    for (struct dirent *d = readdir(rooms_dir);
        d != NULL;
        d = readdir(rooms_dir)) {
        if (*d->d_name == '.') continue;
        result++;
    }

    closedir(rooms_dir);
    return result;
}

void Game::render_player_hud(SDL_Renderer *renderer)
{
    char buffer[256];
    String_Buffer sbuffer = {};
    sbuffer.capacity = sizeof(buffer);
    sbuffer.data = buffer;

    sprintln(&sbuffer, "Dirt blocks: ", entities[PLAYER_ENTITY_INDEX].dirt_blocks_count);

    auto hud_position = vec2(PLAYER_HUD_MARGIN, PLAYER_HUD_MARGIN);
    auto font_size = vec2(PLAYER_HUD_FONT_SIZE, PLAYER_HUD_FONT_SIZE);
    auto board_rect =
        rect(
            hud_position,
            debug_font.text_size(font_size, buffer) + 2.0f * vec2(PLAYER_HUD_PADDING, PLAYER_HUD_PADDING));
    fill_rect(renderer, board_rect, PLAYER_HUD_BACKGROUND_COLOR);

    debug_font.render(
        renderer,
        hud_position + vec2(PLAYER_HUD_PADDING, PLAYER_HUD_PADDING),
        font_size,
        PLAYER_HUD_FONT_COLOR,
        buffer);
}

void Game::render_time_bomb_radius(SDL_Renderer *renderer)
{
    sec(SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255));
    draw_circle(renderer, camera.to_screen(time_bomb), floor(sqrt(TIME_BOMB_RADIUS)));
}

float Game::apply_time_bomb(float dt, Vec2f pos)
{
    return dt * clamp(sqr_dist(pos, time_bomb) / TIME_BOMB_RADIUS, 0.0001f, 1.0f);
}

// https://stackoverflow.com/questions/38334081/howto-draw-circles-arcs-and-vector-graphics-in-sdl
void Game::draw_circle(SDL_Renderer * renderer, Vec2f centre, int radius)
{
   const int diameter = (radius * 2);

   int x = (radius - 1);
   int y = 0;
   int tx = 1;
   int ty = 1;
   int error = (tx - diameter);

   while (x >= y)
   {
      //  Each of the following renders an octant of the circle
      SDL_RenderDrawPoint(renderer, centre.x + x, centre.y - y);
      SDL_RenderDrawPoint(renderer, centre.x + x, centre.y + y);
      SDL_RenderDrawPoint(renderer, centre.x - x, centre.y - y);
      SDL_RenderDrawPoint(renderer, centre.x - x, centre.y + y);
      SDL_RenderDrawPoint(renderer, centre.x + y, centre.y - x);
      SDL_RenderDrawPoint(renderer, centre.x + y, centre.y + x);
      SDL_RenderDrawPoint(renderer, centre.x - y, centre.y - x);
      SDL_RenderDrawPoint(renderer, centre.x - y, centre.y + x);

      if (error <= 0)
      {
         ++y;
         error += ty;
         ty += 2;
      }

      if (error > 0)
      {
         --x;
         tx += 2;
         error += (tx - diameter);
      }
   }
}