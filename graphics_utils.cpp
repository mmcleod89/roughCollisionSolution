#include "graphics_utils.h"
#include <mpi.h>
#include <stdexcept>

using namespace std;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

void DrawCircle(SDL_Renderer *renderer, float centreX, float centreY, float radius)
{
    const int diameter = (radius * 2);

    int x = (radius - 1);
    int y = 0;
    int tx = 1;
    int ty = 1;
    int error = (tx - diameter);

    std::vector<SDL_FPoint> points;

    while (x >= y)
    {
        //  Each of the following renders an octant of the circle
        points.push_back({centreX + x, centreY - y});
        points.push_back({centreX + x, centreY + y});
        points.push_back({centreX - x, centreY - y});
        points.push_back({centreX - x, centreY + y});
        points.push_back({centreX + y, centreY - x});
        points.push_back({centreX + y, centreY + x});
        points.push_back({centreX - y, centreY - x});
        points.push_back({centreX - y, centreY + x});

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
    SDL_RenderPoints(renderer, points.data(), points.size());
}

void CheckForEvents(bool &keep_going, int n_proc, int rank)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            keep_going = false;
        }
    }
    for (int i = 0; i < n_proc; i++)
    {
        if (i != rank)
        {
            MPI_Send(&keep_going, 1, MPI::BOOL, i, 0, MPI_COMM_WORLD);
        }
    }
}

void SetupRendering(int rank, unsigned int w, unsigned int h)
{
    SDL_SetAppMetadata("Collider renderer", "1.0", "collider.mpi");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        throw runtime_error("Failed to initialise SDL");
    }

    if (!SDL_CreateWindowAndRenderer(("Collider Renderer rank " + to_string(rank)).c_str(), w*10, h*10, 0, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        throw runtime_error("Failed to create window / renderer");
    }
}

void RenderScene(std::vector<Body> &bodies, unsigned int X_OFFSET)
{
    float x_offset = float(X_OFFSET);
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    const float red = (float)(0.5 + 0.5);                                            // * SDL_sin(now));
    const float green = (float)(0.5 + 0.5);                                          // * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float)(0.5 + 0.5);                                           // * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT); /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);
    
    for (auto &b : bodies)
    {
        SDL_SetRenderDrawColorFloat(renderer, b.colour[0], b.colour[1], b.colour[2], SDL_ALPHA_OPAQUE_FLOAT);
        DrawCircle(renderer, (b.position[0] - x_offset) * 10, b.position[1] * 10, 10);
    }

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);
}

void CheckForTerminationSignal(int n_proc, int rank, bool &keep_going)
{
    for (int i = 0; i < n_proc; i++)
    {
        if (i != rank)
        {
            MPI_Recv(&keep_going, 1, MPI::BOOL, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}