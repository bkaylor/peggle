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
#define BALL_RADIUS 10
#define PEG_RADIUS 15
#define LAUNCHER_RADIUS 45 

typedef struct {
    vec2 position;
    vec2 velocity;
    float radius;
} Ball;

typedef enum {
    BLUE,
    ORANGE,
    GREEN
} Peg_Type;

typedef struct {
    vec2 position;
    Peg_Type type;
    bool hit;
    float radius;
} Peg;

typedef struct {
    int x;
    int y;
} Window;

// TODO(bkaylor): Multiple pegs (more important), multiple balls (less important)
typedef struct {
    Ball ball;
    Peg pegs[128];
    int peg_count;
    int balls_available;
    bool quit;
    bool reset;
    Window window;
    bool shoot_ball;
    vec2 mouse_vector;
    vec2 ball_launch_position;
    float timer;
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


    Ball ball = game_state.ball;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    draw_circle(renderer, ball.position.x, ball.position.y, ball.radius);

    for (int i = 0; i < game_state.peg_count; i += 1)
    {
        Peg peg = game_state.pegs[i];
        if (peg.hit) 
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
        }

        draw_circle(renderer, peg.position.x, peg.position.y, peg.radius);
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
    draw_circle(renderer, 
            game_state.ball_launch_position.x, 
            game_state.ball_launch_position.y, 
            LAUNCHER_RADIUS);

    // UI
    char balls_available_string[5];
    sprintf(balls_available_string, "%d", game_state.balls_available);
    draw_text(renderer, 
            game_state.ball_launch_position.x, 
            game_state.ball_launch_position.y - 10,
            balls_available_string,
            font,
            font_color);

    char score_string[5];
    int score = 0;
    for (int i = 0; i < game_state.peg_count; i += 1) 
    {
        if (game_state.pegs[i].hit) score += 1; 
    }
    sprintf(score_string, "%d/%d", score, game_state.peg_count);
    draw_text(renderer, 0, 0, score_string, font, font_color);

    SDL_RenderPresent(renderer);
}

Peg make_peg(vec2 position)
{
    return (Peg){{position.x, position.y}, BLUE, false, PEG_RADIUS};
}

void update(Game_State *game_state, float dt) 
{
    vec2 initial_position;
    initial_position.x = game_state->window.x/2; 
    initial_position.y = game_state->window.y-10;
    if (game_state->reset)
    {
        game_state->peg_count = 0;
        game_state->reset = false;
        game_state->shoot_ball = false;
        game_state->ball_launch_position = initial_position;
        game_state->balls_available = 8;

        for (int i = 0; i < 50; i += 1)
        {
            game_state->pegs[i] = make_peg((vec2){
                    initial_position.x + (rand() % 400) - 200,
                    initial_position.y + (rand() % 600) - 800
            });

            game_state->peg_count += 1;
        }
    }

    // TODO(bkaylor): The launcher should move left -> right -> left -> right, not right constantly.
    game_state->timer += dt;
    // if (game_state->ball_launch_position.x >= game_state->window.x - LAUNCHER_RADIUS)
    game_state->ball_launch_position.x = (fmod(game_state->timer,5.0f)/5.0f * game_state->window.x);

    if (game_state->shoot_ball && game_state->balls_available > 0) 
    {
        Ball *ball = &game_state->ball;
        ball->position = game_state->ball_launch_position; 
        ball->velocity = vec2_scalar_multiply(game_state->mouse_vector, -440.0f);
        ball->radius = BALL_RADIUS;

        game_state->shoot_ball = false;

        game_state->balls_available -= 1;
    }

    Ball *ball = &game_state->ball;
    ball->position.x += ball->velocity.x * dt;
    ball->position.y += ball->velocity.y * dt;

    // Check for peg collisions
    // TODO(bkaylor): Ball can get stuck in a peg. Should only allow one collision per frame and shouldn't let ball position go inside a peg. 
    for (int i = 0; i < game_state->peg_count; i += 1)
    {
        Peg *peg = &game_state->pegs[i];
        float dx = (ball->position.x - peg->position.x); 
        float dy = (ball->position.y - peg->position.y);
        float distance_between_ball_and_peg = sqrt((dx*dx) + (dy*dy));
        if (distance_between_ball_and_peg < ball->radius + peg->radius) 
        {
            peg->hit = true;

            float collision_point_x = ((ball->position.x * peg->radius) + (peg->position.x * ball->radius)) / (ball->radius + peg->radius);
            float collision_point_y = ((ball->position.y * peg->radius) + (peg->position.y * ball->radius)) / (ball->radius + peg->radius);

            vec2 normal = vec2_normalize((vec2){peg->position.x - collision_point_x, peg->position.y - collision_point_y});
            vec2 incidence_vector = ball->velocity;

            // TODO(bkaylor): Derive this?
            // Rr = Ri - 2 N (Ri . N)
            ball->velocity = vec2_subtract(incidence_vector, vec2_scalar_multiply(vec2_scalar_multiply(normal, 2), vec2_dot_product(incidence_vector, normal)));

            // Just some friction on collision.
            ball->velocity = vec2_scalar_multiply(ball->velocity, 0.95);
        }
    }

    // TODO(bkaylor): Check for wall collisions

    // TODO(bkaylor): Check for launcher collisions

    // Gravity.
    ball->velocity.y += (140.0f * dt);
}

// TODO(bkaylor): Mouse input.
void get_input(Game_State *game_state, SDL_Renderer *ren)
{
    int x, y;
    SDL_GetMouseState(&x, &y);

    // Handle events.
    SDL_Event event;

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

                    case SDLK_s:
                        game_state->shoot_ball = true;
                        break;

                    default:
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                game_state->mouse_vector = vec2_normalize((vec2){
                    (game_state->ball_launch_position.x) - x,
                    (game_state->ball_launch_position.y) - y,
                });

                game_state->shoot_ball = true; // event.button.button to grab which button.
                break;

            case SDL_QUIT:
                game_state->quit = true;
                break;

            default:
                break;
        }
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

    // SDL_ShowCursor(SDL_DISABLE);

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
            // printf("%.3f\n", delta_t);
        }
    }

	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
    return 0;
}
