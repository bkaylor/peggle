#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "vec2.h"

#define MAX_PEGS 128
#define PI 3.14159265
#define BALL_RADIUS 9
#define PEG_RADIUS 12
#define NET_RADIUS 4
#define LAUNCHER_RADIUS 50 
#define ANIMATION_PEG_SHRINKING_TIME 130
#define ANIMATION_BALL_SHRINKING_TIME 60
#define MESSAGE_TIMER 1
#define NET_COOLDOWN 3

typedef enum {
    ANIMATION_SHRINKING,
    ANIMATION_NONE
} Animation_Type;

typedef struct {
    Animation_Type type;
    int total_time;
    int time_left;
} Animation_Info;

typedef struct {
    vec2 position;
    vec2 velocity;
    float radius;
    float starting_radius;
    Animation_Info animation;
    bool captured;
    bool out_of_play;
} Ball;

typedef struct {
    vec2 position;
    vec2 velocity;
    float radius;
    bool out_of_play;
    Animation_Info animation;
} Net;

typedef enum {
    NORMAL_PEG,
    REQUIRED_PEG,
    SPECIAL_PEG
} Peg_Type;

typedef enum {
    RANDOM_CLEAR_SPECIAL,
    EXTRA_BALL_SPECIAL,
    DUPLICATE_BALL_SPECIAL,
    NONE_SPECIAL
} Special_Peg_Type;

typedef struct {
    vec2 position;
    Peg_Type type;
    Special_Peg_Type special;
    bool special_has_been_claimed;
    bool hit;
    float radius;
    float starting_radius;
    Animation_Info animation;
} Peg;

typedef struct {
    vec2 position;
    vec2 velocity;
    float radius;
    float starting_radius;
    Animation_Info animation;

    float visible_net_cooldown_radius;
} Launcher;

typedef struct {
    int x;
    int y;
} Window;

typedef enum {
    START_SCREEN,
    GAME_SCREEN,
    WIN_SCREEN
} Screen;

typedef enum {
    EXTRA_BALL_MESSAGE,
    FREE_PEG_MESSAGE,
    DUPLICATE_BALL_MESSAGE,
    NET_AVAILABLE_MESSAGE,
    LOSE_MESSAGE,
    NONE_MESSAGE
} Message;

typedef struct {
    Ball ball[256];
    int ball_count;
    int balls_available;
    bool shoot_ball;

    Peg pegs[256];
    int peg_count;

    Net nets[256];
    int net_count;
    bool shoot_net;
    bool net_available;
    float net_cooldown;
    float net_cooldown_max;

    bool quit;
    bool reset;
    Window window;
    vec2 mouse_vector;
    float timer;
    Screen screen;

    int score;
    int required_peg_count;

    Message message;
    float message_timer;

    Launcher launcher;
} Game_State;

void draw_text(SDL_Renderer *renderer, int x, int y, char *string, TTF_Font *font, SDL_Color font_color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, string, font_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    int x_from_texture, y_from_texture;
    SDL_QueryTexture(texture, NULL, NULL, &x_from_texture, &y_from_texture);
    SDL_Rect rect = {x, y, x_from_texture, y_from_texture};

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// TODO(bkaylor): This is stolened.
void draw_circle(SDL_Renderer *renderer, int32_t centreX, int32_t centreY, int32_t radius)
{
   const int32_t diameter = (radius * 2);

   int32_t x = (radius - 1);
   int32_t y = 0;
   int32_t tx = 1;
   int32_t ty = 1;
   int32_t error = (tx - diameter);

   while (x >= y)
   {
      //  Each of the following renders an octant of the circle
      SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

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

void render(SDL_Renderer *renderer, Game_State game_state, TTF_Font *font, SDL_Color font_color)
{
    SDL_RenderClear(renderer);

    // Set background color.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, NULL);


    switch (game_state.screen)
    {
        case GAME_SCREEN:
            for (int i = 0; i < game_state.ball_count; i += 1)
            {
                Ball ball = game_state.ball[i];
                if (ball.captured) continue; 

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
                draw_circle(renderer, ball.position.x, ball.position.y, ball.radius);
            }

            for (int i = 0; i < game_state.net_count; i += 1)
            {
                Net net = game_state.nets[i];
                if (net.out_of_play) continue;
                SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
                draw_circle(renderer, net.position.x, net.position.y, net.radius);
            }

            for (int i = 0; i < game_state.peg_count; i += 1)
            {
                Peg peg = game_state.pegs[i];
                if (peg.hit) continue; 

                switch (peg.type) {
                    case REQUIRED_PEG:
                        SDL_SetRenderDrawColor(renderer, 224, 143, 67, 0);
                    break;
                    case SPECIAL_PEG:
                        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
                    break;
                    case NORMAL_PEG:
                    default:
                        SDL_SetRenderDrawColor(renderer, 50, 50, 255, 0);
                    break;
                }

                draw_circle(renderer, peg.position.x, peg.position.y, peg.radius);
            }

            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
            draw_circle(renderer, game_state.launcher.position.x, game_state.launcher.position.y, game_state.launcher.radius);

            if (!game_state.net_available) {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 0);
                draw_circle(renderer, game_state.launcher.position.x, game_state.launcher.position.y, game_state.launcher.visible_net_cooldown_radius);
            }

            // UI
            char balls_available_string[5];
            sprintf(balls_available_string, "%d", game_state.balls_available);
            draw_text(renderer, 
                    game_state.launcher.position.x - 10, 
                    game_state.launcher.position.y- 10,
                    balls_available_string,
                    font,
                    font_color);

            char score_string[5];
            int score = 0;
            int required_pegs = 0;

            sprintf(score_string, "%d/%d", game_state.score, game_state.required_peg_count);
            draw_text(renderer, 0, 0, score_string, font, font_color);

            if (game_state.message != NONE_MESSAGE) {
                char gameplay_message[50];

                switch (game_state.message)
                {
                    case EXTRA_BALL_MESSAGE: {
                        sprintf(gameplay_message, "Extra ball");
                    } break;

                    case FREE_PEG_MESSAGE: {
                        sprintf(gameplay_message, "Free orange peg");
                    } break;

                    case DUPLICATE_BALL_MESSAGE: {
                        sprintf(gameplay_message, "Multiball");
                    } break;

                    case NET_AVAILABLE_MESSAGE: {
                        sprintf(gameplay_message, "Net ready");
                    } break;

                    case LOSE_MESSAGE: {
                        sprintf(gameplay_message, "You lost. R to restart!");
                    } break;

                    default: {
                        sprintf(gameplay_message, "Unimplemented message");
                    } break;
                }

                draw_text(renderer, game_state.window.x/2 - 50, game_state.window.y/2 - 50, gameplay_message, font, font_color);
            }
        break;

        case START_SCREEN:
            char title_message[50];
            sprintf(title_message, "PEGGLE");

            char start_message[50];
            sprintf(start_message, "Click to play!");

            draw_text(renderer, game_state.window.x/2 - 50, game_state.window.y/2 - 50, title_message, font, font_color);
            draw_text(renderer, game_state.window.x/2 - 50, game_state.window.y/2 + 50, start_message, font, font_color);
        break;
        case WIN_SCREEN:
            char win_title_message[50];
            sprintf(title_message, "WIN");

            char win_start_message[50];
            sprintf(start_message, "Click to play again!");

            draw_text(renderer, game_state.window.x/2 - 50, game_state.window.y/2 - 50, title_message, font, font_color);
            draw_text(renderer, game_state.window.x/2 - 50, game_state.window.y/2 + 50, start_message, font, font_color);
        break;
    }

    SDL_RenderPresent(renderer);
}

Ball make_ball(vec2 position, vec2 velocity) {
    Ball ball;
    ball.position = position;
    ball.velocity = velocity;
    ball.radius = BALL_RADIUS;
    ball.starting_radius = ball.radius;
    ball.captured = false;
    ball.out_of_play = false;
    ball.animation.type = ANIMATION_NONE;

    return ball;
}

Peg make_peg(vec2 position, Peg_Type type)
{
    Peg peg;
    peg.position = position;
    peg.type = type;
    peg.hit = false;
    peg.radius = PEG_RADIUS;
    peg.starting_radius = peg.radius;
    peg.animation.type = ANIMATION_NONE;

    if (type == SPECIAL_PEG) {
        int d3 = rand() % 3;
        switch (d3)
        {
            case 0:
                peg.special = RANDOM_CLEAR_SPECIAL;
            break;
            case 1:
                peg.special = EXTRA_BALL_SPECIAL;
            break;
            case 2:
                peg.special = DUPLICATE_BALL_SPECIAL;
            break;
            default:
                peg.special = NONE_SPECIAL;
            break;
        }
        peg.special_has_been_claimed = false;
    } else {
        peg.special = NONE_SPECIAL;
    }

    return peg;
}

void set_peg_to_hit(Peg *peg)
{
    if (peg->animation.type == ANIMATION_NONE) {
        peg->animation.type = ANIMATION_SHRINKING;
        peg->animation.total_time = ANIMATION_PEG_SHRINKING_TIME;
        peg->animation.time_left = peg->animation.total_time;
    }
}

void show_message(Game_State *game_state, Message message)
{
    game_state->message = message;
    game_state->message_timer = MESSAGE_TIMER;
}

void update(Game_State *game_state, float dt) 
{
    if (game_state->screen != GAME_SCREEN) return;

    vec2 initial_position;
    initial_position.x = game_state->window.x/2; 
    initial_position.y = game_state->window.y-10;

    if (game_state->reset)
    {
        game_state->peg_count = 0;
        game_state->ball_count = 0;
        game_state->net_count = 0;

        game_state->reset = false;
        game_state->shoot_ball = false;
        game_state->balls_available = 8;
        game_state->net_available = true;
        game_state->message = NONE_MESSAGE;

        for (int i = 0; i < 50; i += 1)
        {
            vec2 position = {
                initial_position.x + (rand() % 400) - 200,
                initial_position.y + (rand() % 600) - 800
            };

            Peg_Type type;
            if (i < 30) {
                type = NORMAL_PEG;
            } else if (i < 40) {
                type = REQUIRED_PEG;
            } else if (i < 50) {
                type = SPECIAL_PEG;
            }

            game_state->pegs[i] = make_peg(position, type);

            game_state->peg_count += 1;
        }

        game_state->required_peg_count = 0;
        game_state->score = 0;

        for (int i = 0; i < game_state->peg_count; i += 1)
        {
            if (game_state->pegs[i].type == REQUIRED_PEG) {
                game_state->required_peg_count += 1;
            }
        }

        game_state->launcher.position = initial_position;
        game_state->launcher.velocity = vec2_make(150.0f, 0.0f);
        game_state->launcher.radius = LAUNCHER_RADIUS; 
        game_state->launcher.animation.type = ANIMATION_NONE; 
    }

    if (game_state->balls_available == 0) dt /= 3;

    game_state->timer += dt;

    //
    // Update launcher
    //
    Launcher *launcher = &game_state->launcher;
    launcher->position.x += launcher->velocity.x * dt;
    launcher->position.y += launcher->velocity.y * dt;

    if (launcher->position.x + launcher->radius >= game_state->window.x ||
        launcher->position.x - launcher->radius <= 0) {
        launcher->velocity = vec2_scalar_multiply(launcher->velocity, -1.0f);
        launcher->position = vec2_add(launcher->position, vec2_scalar_multiply(launcher->velocity, 0.05f)); // Bump the launcher position so it doesn't get stuck in the wall.
    }

    if (!game_state->net_available) {
        launcher->visible_net_cooldown_radius = launcher->radius * ((game_state->net_cooldown_max - game_state->net_cooldown) / game_state->net_cooldown_max);
    }

    //
    // Shoot ball
    //
    if (game_state->shoot_ball && game_state->balls_available > 0) 
    {
        Ball *ball = &game_state->ball[game_state->ball_count];

        game_state->ball[game_state->ball_count] = make_ball(vec2_subtract(game_state->launcher.position, vec2_scalar_multiply(vec2_normalize(game_state->mouse_vector), game_state->launcher.radius + 10.0f)), vec2_scalar_multiply(game_state->mouse_vector, -465.0f));

        game_state->shoot_ball = false;

        game_state->ball_count += 1;
        game_state->balls_available -= 1;
    }

    // Shoot net
    if (game_state->shoot_net && game_state->net_available)
    {
        Net *net = &game_state->nets[game_state->net_count];
        net->position = game_state->launcher.position;
        net->velocity = vec2_scalar_multiply(game_state->mouse_vector, -1000.0f);
        net->radius = NET_RADIUS;
        net->out_of_play = false;

        game_state->net_cooldown = NET_COOLDOWN;
        game_state->net_cooldown_max = game_state->net_cooldown;
        game_state->net_available = false;

        game_state->net_count += 1;

        game_state->shoot_net = false;
    }

    // Update all balls
    for (int ball_index = 0; ball_index < game_state->ball_count; ball_index += 1)
    {
        Ball *ball = &game_state->ball[ball_index];
        if (ball->out_of_play) continue;

        ball->position.x += ball->velocity.x * dt;
        ball->position.y += ball->velocity.y * dt;

        // Check for ball->peg collisions
        for (int i = 0; i < game_state->peg_count; i += 1)
        {
            Peg *peg = &game_state->pegs[i];
            if (peg->hit) continue;

            float dx = (ball->position.x - peg->position.x); 
            float dy = (ball->position.y - peg->position.y);
            float distance_between_ball_and_peg = sqrt((dx*dx) + (dy*dy));
            if (distance_between_ball_and_peg < ball->radius + peg->radius) 
            {
                set_peg_to_hit(peg);

                float collision_point_x = ((ball->position.x * peg->radius) + (peg->position.x * ball->radius)) / (ball->radius + peg->radius);
                float collision_point_y = ((ball->position.y * peg->radius) + (peg->position.y * ball->radius)) / (ball->radius + peg->radius);

                vec2 normal = vec2_normalize((vec2){peg->position.x - collision_point_x, peg->position.y - collision_point_y});
                vec2 incidence_vector = ball->velocity;

                // TODO(bkaylor): Derive this?
                // Rr = Ri - 2 N (Ri . N)
                ball->velocity = vec2_subtract(incidence_vector, vec2_scalar_multiply(vec2_scalar_multiply(normal, 2), vec2_dot_product(incidence_vector, normal)));

                // Bump the ball position to avoid it getting stuck.
                ball->position = vec2_subtract(ball->position, vec2_scalar_multiply(normal, 0.1f));

                // A bit of friction on the ball.
                ball->velocity = vec2_scalar_multiply(ball->velocity, 0.95);

                // Handle special pegs
                if (peg->type == SPECIAL_PEG && !peg->special_has_been_claimed)
                {
                    switch (peg->special) {
                        case EXTRA_BALL_SPECIAL:
                            game_state->balls_available += 1;
                            show_message(game_state, EXTRA_BALL_MESSAGE);
                        break;
                        case RANDOM_CLEAR_SPECIAL:
                            for (int i = 0; i < game_state->peg_count; i += 1) {
                                if (game_state->pegs[i].type == REQUIRED_PEG && !game_state->pegs[i].hit) {
                                    set_peg_to_hit(&game_state->pegs[i]);
                                    show_message(game_state, FREE_PEG_MESSAGE);
                                    break;
                                }

                            }
                        break;
                        case DUPLICATE_BALL_SPECIAL:
                            game_state->ball[game_state->ball_count] = make_ball(ball->position, vec2_scalar_multiply(ball->velocity, 0.8f));
                            game_state->ball_count += 1;

                            show_message(game_state, DUPLICATE_BALL_MESSAGE);
                        case NONE_SPECIAL:
                        default:
                        break;
                    }

                    peg->special_has_been_claimed = true;
                }
            }
        }

        // Check for wall collisions
        if ((ball->position.x + ball->radius) > game_state->window.x || (ball->position.x - ball->radius) < 0) ball->velocity.x *= -1;
        if ((ball->position.y - ball->radius) < 0) ball->velocity.y *= -1;

        // Check for launcher collisions
        float dx = (ball->position.x - launcher->position.x); 
        float dy = (ball->position.y - launcher->position.y);
        float distance_between_ball_and_launcher = sqrt((dx*dx) + (dy*dy));
        if (distance_between_ball_and_launcher < ball->radius + launcher->radius) 
        {
            float collision_point_x = ((ball->position.x * launcher->radius) + (launcher->position.x * ball->radius)) / (ball->radius + launcher->radius);
            float collision_point_y = ((ball->position.y * launcher->radius) + (launcher->position.y * ball->radius)) / (ball->radius + launcher->radius);

            vec2 normal = vec2_normalize((vec2){launcher->position.x - collision_point_x, launcher->position.y - collision_point_y});
            vec2 incidence_vector = ball->velocity;

            // TODO(bkaylor): Derive this?
            // Rr = Ri - 2 N (Ri . N)
            ball->velocity = vec2_subtract(incidence_vector, vec2_scalar_multiply(vec2_scalar_multiply(normal, 2), vec2_dot_product(incidence_vector, normal)));

            // Bump the ball position to avoid it getting stuck.
            ball->position = vec2_subtract(ball->position, vec2_scalar_multiply(normal, 0.1f));

            // A bit of bounce on the ball.
            ball->velocity = vec2_scalar_multiply(ball->velocity, 1.3f);
        }

        // Gravity.
        ball->velocity.y += (140.0f * dt);

        if ((ball->position.y - ball->radius) > game_state->window.y) ball->out_of_play = true;

        // Update ball animations
        if (ball->animation.type != ANIMATION_NONE) {
            ball->animation.time_left -= dt;

            if (ball->animation.type == ANIMATION_SHRINKING) {
                /*if (ball->animation.time_left < 30)*/ ball->radius = ball->starting_radius * ((float)ball->animation.time_left / (float)ball->animation.total_time);
                if (ball->animation.time_left <= 0) {
                    ball->animation.type = ANIMATION_NONE;
                    ball->radius = 0;
                    ball->captured = true;
                    ball->out_of_play = true;
                    game_state->balls_available += 1;
                }
            }
        }
    }

    // Update all pegs
    for (int peg_index = 0; peg_index < game_state->peg_count; peg_index += 1)
    {
        Peg *peg = &game_state->pegs[peg_index];
        if (peg->hit) continue;

        // Update peg animations
        if (peg->animation.type != ANIMATION_NONE) {
            peg->animation.time_left -= dt;

            if (peg->animation.type == ANIMATION_SHRINKING) {
                peg->radius = peg->starting_radius * ((float)peg->animation.time_left / (float)peg->animation.total_time);
                if (peg->animation.time_left <= 0) {
                    peg->animation.type = ANIMATION_NONE;
                    peg->radius = 0;
                    peg->hit = true;
                    if (peg->type == REQUIRED_PEG) {
                        game_state->score += 1;

                        if (game_state->score == game_state->required_peg_count) {
                            game_state->screen = WIN_SCREEN;
                        }
                    }
                }
            }
        }
    }

    // Update all nets 
    for (int net_index = 0; net_index < game_state->net_count; net_index += 1)
    {
        Net *net = &game_state->nets[net_index];
        if (net->out_of_play) continue;
        net->position.x += net->velocity.x * dt;
        net->position.y += net->velocity.y * dt;

        // Check for net->ball collisions
        for (int i = 0; i < game_state->ball_count; i += 1)
        {
            Ball *ball = &game_state->ball[i];
            if (ball->out_of_play || ball->captured) continue;

            float dx = (net->position.x - ball->position.x); 
            float dy = (net->position.y - ball->position.y);
            float distance_between_net_and_ball = sqrt((dx*dx) + (dy*dy));
            if (distance_between_net_and_ball < net->radius + ball->radius) 
            {
                // Set the ball hit state
                if (ball->animation.type == ANIMATION_NONE) {
                    ball->animation.type = ANIMATION_SHRINKING;
                    ball->animation.total_time = ANIMATION_BALL_SHRINKING_TIME;
                    ball->animation.time_left = ball->animation.total_time;

                    ball->velocity = vec2_scalar_multiply(ball->velocity, 0.15f);
                }

            }
        }

        // Check for net->peg collisions
        for (int i = 0; i < game_state->peg_count; i += 1)
        {
            Peg *peg = &game_state->pegs[i];
            if (peg->hit) continue;

            float dx = (net->position.x - peg->position.x); 
            float dy = (net->position.y - peg->position.y);
            float distance_between_net_and_peg = sqrt((dx*dx) + (dy*dy));
            if (distance_between_net_and_peg < net->radius + peg->radius) 
            {
                net->out_of_play = true;
            }
        }

        // Gravity.
        net->velocity.y += (140.0f * dt);
    }

    // Update gameplay message 
    if (game_state->message != NONE_MESSAGE) {
        if (game_state->message_timer <= 0) {
            game_state->message = NONE_MESSAGE;
        } else {
            game_state->message_timer -= dt;
        }
    }

    // Update net timer
    if (!game_state->net_available) {
        game_state->net_cooldown -= dt;

        if (game_state->net_cooldown <= 0) {
            game_state->net_available = true;
            show_message(game_state, NET_AVAILABLE_MESSAGE);
        }
    }
    
    // Check lose conditions
    if (game_state->balls_available <= 0) {
        int balls_in_play = 0;
        for (int i = 0; i < game_state->ball_count; i += 1)
        {
            if (!game_state->ball[i].out_of_play) {
                balls_in_play += 1;
                break;
            }
        }

        if (balls_in_play == 0) {
            show_message(game_state, LOSE_MESSAGE);
        }
    }
}

void get_input(Game_State *game_state, SDL_Renderer *ren)
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    // SDL_GetRelativeMouseState(&x, &y);

    // Handle events.
    SDL_Event event;

    switch (game_state->screen)
    {
        case GAME_SCREEN:
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:
                                game_state->screen = START_SCREEN;
                                break;

                            case SDLK_r:
                                game_state->reset = true;
                                break;

                            case SDLK_s:
                                game_state->shoot_ball = true;
                                break;

                            default:
                                break;
                        }
                        break;

                    case SDL_MOUSEBUTTONDOWN:
                        game_state->mouse_vector = vec2_normalize((vec2){
                            (game_state->launcher.position.x) - x,
                            (game_state->launcher.position.y) - y,
                        });

                        if (event.button.button == SDL_BUTTON_LEFT && game_state->balls_available > 0) {
                            game_state->shoot_ball = true; 
                        }

                        if (event.button.button == SDL_BUTTON_RIGHT && game_state->net_available) {
                            game_state->shoot_net = true;
                        }
                        break;

                    case SDL_QUIT:
                        game_state->quit = true;
                        break;

                    default:
                        break;
                }
            }
        break;
        
        case WIN_SCREEN:
        case START_SCREEN:
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:
                                game_state->quit = true;
                                break;

                            case SDLK_r:
                                game_state->reset = true;
                                break;
                            default:
                                break;
                        }
                        break;

                    case SDL_MOUSEBUTTONDOWN:
                        game_state->screen = GAME_SCREEN;
                        game_state->reset = true;
                        break;

                    case SDL_QUIT:
                        game_state->quit = true;
                        break;

                    default:
                        break;
                }
            }
        break;
    }
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init video error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init audio error: %s\n", SDL_GetError());
        return 1;
    }

    /*
    SDL_ShowCursor(SDL_ENABLE);
    SDL_CaptureMouse(SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    */

	// Setup window
	SDL_Window *win = SDL_CreateWindow("Peggle",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			600, 800,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Setup renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Setup font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("liberation.ttf", 16);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), win);
		return -666;
	}
	SDL_Color font_color = {255, 255, 255};

    // Setup main loop
    srand(time(NULL));

    Game_State game_state = {0};
    game_state.reset = 1;

    // Main loop
    const float FPS_INTERVAL = 1.0f;
    Uint64 fps_start, fps_current, fps_frames = 0;

    int frame_time_start, frame_time_finish;
    float delta_t = 0;

    while (!game_state.quit)
    {
        frame_time_start = SDL_GetTicks();

        SDL_PumpEvents();
        get_input(&game_state, ren);

        if (!game_state.quit)
        {
            SDL_GetWindowSize(win, &game_state.window.x, &game_state.window.y);

            update(&game_state, delta_t);
            render(ren, game_state, font, font_color);

            frame_time_finish = SDL_GetTicks();
            delta_t = (float)((frame_time_finish - frame_time_start) / 1000.0f);
        }
    }

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
    return 0;
}
