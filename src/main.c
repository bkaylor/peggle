#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#define PI 3.14159265
#define RADIUS 10

typedef struct {
    float x;
    float y;
} v2;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    v2 position;
    v2 velocity;
} Ball;

typedef enum {
    BLUE,
    ORANGE,
    GREEN
} Peg_Type;

typedef struct {
    v2 position;
    Peg_Type type;
} Peg;

typedef struct {
    int x;
    int y;
} Window;

typedef struct {
    Ball ball;
    Peg peg;
    int balls_available;
    bool quit;
    bool reset;
    Window window;
} Game_State;

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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);

    Ball ball = game_state.ball;
    Peg peg = game_state.peg;
    draw_circle(renderer, ball.position.x, ball.position.y, RADIUS);
    draw_circle(renderer, peg.position.x, peg.position.y, RADIUS);

    SDL_RenderPresent(renderer);
}

void update(Game_State *game_state, float dt) 
{
    if (game_state->reset)
    {
        Position initial_position;
        initial_position.x = game_state->window.x - 10; 
        initial_position.x = game_state->window.y/2;

        Ball ball;
        ball.position.x = initial_position.x;
        ball.position.y = initial_position.y; 
        ball.velocity.x = 0.0f;
        ball.velocity.y = -200.0f;

        game_state->ball = ball;

        Peg peg;
        peg.position.x = initial_position.x;
        peg.position.y = initial_position.y - (game_state->window.y/2);

        game_state->peg = peg;

        game_state->reset = false;
    }

    Ball *ball = &game_state->ball;
    ball->position.x += ball->velocity.x * dt;
    ball->position.y += ball->velocity.y * dt;

    // Gravity.
    ball->velocity.y += (80.0f * dt);
}

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
