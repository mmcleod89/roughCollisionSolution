/* clear.c ... */

/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

// #define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#ifdef GRAPHICS
#include "graphics_utils.h"
#endif 

#include <vector>
#include <mpi.h>
#include "circle_collider.hpp"
#include <string>
#include <stdexcept>

/* We will use this renderer to draw into this window every frame. */


int main()
{
    MPI_Init(NULL, NULL);
    int rank, n_proc;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_proc);

    const unsigned int WIDTH = 100;
    const unsigned int X_OFFSET = 0; // min value of x in this process; necessary for rendering
    const unsigned int HEIGHT = 50;

    #ifdef GRAPHICS
    SetupRendering(rank, WIDTH, HEIGHT);
    #endif

    Body b1, b2, b3, b4;
    b1.position = {30, 25, 0}; b1.velocity = {1, 0, 0}; b1.colour = {0, 0, 255};
    b2.position = {90, 25, 0}; b2.velocity = {-2, 0, 0}; b2.colour = {255, 0, 0};
    vector<Body> bodies = {b1, b2};

    bool keep_going = true;
    double t = 0;
    double t_max = 100;
    unsigned int steps = 0;
    double dt = 0.1;
    while (keep_going && t < t_max)
    {
        t = (steps++) * dt;
        // update positions
        for (auto &b : bodies)
        {
            b.position = b.position + b.velocity * dt;
        }

        // check for object collisions and bounce back
        for (size_t i = 0; i < bodies.size(); i++)
        {
            for (size_t j = i + 1; j < bodies.size(); j++)
            {
                if (checkCollision(bodies[i], bodies[j]))
                {
                    cout << "Collision on rank " << rank << endl;
                    reverse(bodies[i], bodies[j]);
                }
            }
        }

        // check for wall collisions and bounce back
        for (auto &b : bodies)
        {
            wallBounce(b, WIDTH, HEIGHT);
        }


        // Only necessary for graphical rendering
#ifdef GRAPHICS
        RenderScene(bodies, X_OFFSET);
        if (rank == 0)
        {
            CheckForTerminationSignal(n_proc, rank, keep_going);
            CheckForEvents(keep_going, n_proc, rank);
        }
        else
        {
            CheckForEvents(keep_going, n_proc, rank);
            CheckForTerminationSignal(n_proc, rank, keep_going);
        }
#endif
    }

    // exit
    MPI_Finalize();
    return 0; /* end the program, reporting success to the OS. */
}



