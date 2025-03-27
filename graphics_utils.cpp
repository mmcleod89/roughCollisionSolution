#include "graphics_utils.h"
#include <mpi.h>
#include <cmath>
#include <stdexcept>
#include "MpiTags.h"

using namespace std;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

void DrawCircle(SDL_Renderer *renderer, float centreX, float centreY, float radius)
{
    static vector<SDL_FPoint> circle_template;
    static vector<SDL_FPoint> circle_points;
    if(circle_template.empty())
    {
        int pix_half_width = static_cast<int>(radius * 10);
        int width = 2 * pix_half_width + 1;
        for(int i = 0; i < width; i++)
        {
            for(int j = 0; j < width; j++)
            {
                float x = pix_half_width - i;
                float y = pix_half_width - j;
                if (x*x + y*y <= radius*radius)
                {
                    circle_template.push_back({x, y});
                }
            }
        }
        circle_points = vector<SDL_FPoint>(circle_template.size());
    }

    for(size_t i = 0; i < circle_template.size(); i++)
    {
        circle_points[i] = {circle_template[i].x+centreX, circle_template[i].y+centreY};
    }
    SDL_RenderPoints(renderer, circle_points.data(), circle_points.size());
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
            MPI_Send(&keep_going, 1, MPI::BOOL, i, MessageTags::terminate, MPI_COMM_WORLD);
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

void RenderScene(std::vector<Body> &bodies, unsigned int X_OFFSET, unsigned int X_BUFFER, unsigned int HEIGHT)
{
    float x_offset = float(X_OFFSET);

    const float red = 1.0;
    const float green = 1.0;
    const float blue = 1.0;
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT); 

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);
    
    /* Draw the boundaries of the buffer zone */
    SDL_SetRenderDrawColorFloat(renderer, 1.0, 0.0, 0.0, SDL_ALPHA_OPAQUE_FLOAT);
    float buffer_x_pos = 10.0*(X_BUFFER - X_OFFSET);
    SDL_RenderLine(renderer, buffer_x_pos, 0.0, buffer_x_pos, HEIGHT*10.0);

    /* Draw the bodies */
    for (auto &b : bodies)
    {
        SDL_SetRenderDrawColorFloat(renderer, b.colour[0], b.colour[1], b.colour[2], SDL_ALPHA_OPAQUE_FLOAT);
        DrawCircle(renderer, (b.position[0] - x_offset) * 10, b.position[1] * 10, 10);
    }

    /* Put it on the screen! */
    SDL_RenderPresent(renderer);
}

void CheckForTerminationSignal(int n_proc, int rank, bool &keep_going)
{
    for (int i = 0; i < n_proc; i++)
    {
        if (i != rank)
        {
            MPI_Recv(&keep_going, 1, MPI::BOOL, i, MessageTags::terminate, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}