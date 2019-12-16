#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "vec2.h"

#define PI 3.14159265
#define BALL_RADIUS 10
#define PEG_RADIUS 20

typedef struct {
    int x;
    int y;
} Position;

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
    Peg peg;
    int balls_available;
    bool quit;
    bool reset;
    Window window;
    bool shoot_ball;
} Game_State;

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

void render(SDL_Renderer *renderer, Game_State game_state)
{
    SDL_RenderClear(renderer);

    // Set background color.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderFillRect(renderer, NULL);


    Ball ball = game_state.ball;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    draw_circle(renderer, ball.position.x, ball.position.y, ball.radius);

    Peg peg = game_state.peg;
    if (peg.hit) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
    draw_circle(renderer, peg.position.x, peg.position.y, peg.radius);

    SDL_RenderPresent(renderer);
}

void update(Game_State *game_state, float dt) 
{
    Position initial_position;
    initial_position.x = game_state->window.x/2; 
    initial_position.y = game_state->window.y-10;
    if (game_state->reset)
    {
        Ball *ball = &game_state->ball;
        ball->position.x = initial_position.x + 10;
        ball->position.y = initial_position.y; 
        ball->velocity.x = 0.0f;
        ball->velocity.y = -200.0f;
        ball->radius = BALL_RADIUS;

        Peg *peg = &game_state->peg;
        peg->position.x = initial_position.x;
        peg->position.y = initial_position.y - (game_state->window.y/2);
        peg->hit = false;
        peg->radius = PEG_RADIUS;

        game_state->reset = false;

        game_state->balls_available = 3;
    }

    if (game_state->shoot_ball && game_state->balls_available > 0) 
    {
        Ball *ball = &game_state->ball;
        ball->position.x = initial_position.x + 10;
        ball->position.y = initial_position.y; 
        ball->velocity.x = 0.0f;
        ball->velocity.y = -200.0f;
        ball->radius = BALL_RADIUS;

        game_state->shoot_ball = false;

        game_state->balls_available -= 1;
    }

    Ball *ball = &game_state->ball;
    ball->position.x += ball->velocity.x * dt;
    ball->position.y += ball->velocity.y * dt;

    Peg *peg = &game_state->peg;

    // Check for collision.
    float dx = (ball->position.x - peg->position.x); 
    float dy = (ball->position.y - peg->position.y);
    float distance_between_ball_and_peg = sqrt((dx*dx) + (dy*dy));
    if (distance_between_ball_and_peg < ball->radius + peg->radius) 
    {
        printf("Collided!\n"); 
        peg->hit = true;

        float collision_point_x = ((ball->position.x * peg->radius) + (peg->position.x * ball->radius)) / (ball->radius + peg->radius);
        float collision_point_y = ((ball->position.y * peg->radius) + (peg->position.y * ball->radius)) / (ball->radius + peg->radius);

        vec2 normal = vec2_normalize((vec2){peg->position.x - collision_point_x, peg->position.y - collision_point_y});
        vec2 incidence_vector = ball->velocity;

        // Rr = Ri - 2 N (Ri . N)
        ball->velocity = vec2_subtract(incidence_vector, vec2_scalar_multiply(vec2_scalar_multiply(normal, 2), vec2_dot_product(incidence_vector, normal)));
    }

    // Gravity.
    ball->velocity.y += (80.0f * dt);
}

// TODO(bkaylor): Mouse input.
void get_input(Game_State *game_state, SDL_Renderer *ren)
{
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
			400, 400,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Setup renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Setup font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("liberation.ttf", 12);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), win);
		return -666;
	}

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
            render(ren, game_state);

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
