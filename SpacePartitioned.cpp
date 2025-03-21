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
#include "SpacePartitioned.h"
#include <cmath>
#include <random>

/* We will use this renderer to draw into this window every frame. */


int main()
{
    MPI_Init(NULL, NULL);
    int rank, n_proc;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_proc);

    const unsigned int WIDTH_FULL = 100;
    const unsigned int WIDTH = WIDTH_FULL / n_proc;
    const unsigned int HEIGHT = 50;
    const unsigned int X_OFFSET = WIDTH * rank;

    #ifdef GRAPHICS
    SetupRendering(rank, WIDTH, HEIGHT);
    #endif

    vector<Body> local_bodies;  // bodies in this parition
    vector<Body> foreign_bodies;  // bodies in buffer of neighbouring partition
    vector<Body> buffer_bodies;  // bodies to send to neighbour
    SetupBodies(X_OFFSET, WIDTH, local_bodies);

    Body recv_buffer[100]; // max size of receive buffer

    bool keep_going = true;
    double t = 0;
    double t_max = 200;
    unsigned int steps = 0;
    double dt = 0.1;
    while (keep_going && t < t_max)
    {
        t = (steps++) * dt;
        // update positions
        for (auto &b : local_bodies)
        {
            b.position = b.position + b.velocity * dt;
        }

        // build buffer lists
        float x_boundary = WIDTH;
        buffer_bodies.clear();
        for (auto &b : local_bodies)
        {
            if(std::abs(b.position[0] - x_boundary) < 2.0)
            {
                buffer_bodies.push_back(b);
            }
        }

        // communicate the buffer lists
        if(rank == 0)
        {
            MPI_Send(buffer_bodies.data(), buffer_bodies.size() * sizeof(Body), MPI_BYTE, 1, 1, MPI_COMM_WORLD);
            ReceiveBodies(rank, n_proc, recv_buffer, foreign_bodies);
        }
        else if(rank==1)
        {
            ReceiveBodies(rank, n_proc, recv_buffer, foreign_bodies);
            MPI_Send(buffer_bodies.data(), buffer_bodies.size() * sizeof(Body), MPI_BYTE, 0, 1, MPI_COMM_WORLD);
        }

        // adopt foreign bodies that are inside our region
        for(auto &b : foreign_bodies)
        {
            if(b.position[0] >= float(X_OFFSET) && b.position[0] < float(X_OFFSET + WIDTH))
            {
                cout << "Rank " << rank << " adopted a new body " << b << endl;
                local_bodies.push_back(b);
            }
        }

        //abandon bodies that leave our region
        for(auto &b : buffer_bodies)
        {
            if(b.position[0] >= float(X_OFFSET+WIDTH) || b.position[0] < X_OFFSET)
            {
                cout << "Rank " << rank << " abandoned a body " << b << endl;
                RemoveBody(local_bodies, b);
            }
        }

        // check for object collisions and bounce back
        for (size_t i = 0; i < local_bodies.size(); i++)
        {
            for (size_t j = i + 1; j < local_bodies.size(); j++)
            {
                if (checkCollision(local_bodies[i], local_bodies[j]))
                {
                    cout << "Collision on rank " << rank << endl;
                    reverse(local_bodies[i], local_bodies[j]);
                }
            }
        }

        // check for buffer collisions and bounce back
        for(auto &local : buffer_bodies)
        {
            for(auto &foreign : foreign_bodies)
            {
                if(checkCollision(local, foreign))
                {
                    cout << "Buffer collision on rank " << rank << endl;
                    for(auto &b : local_bodies)
                    {
                        if(b.id == local.id)
                        {
                            reverse(b, foreign);
                        }
                    }
                }
            }
        }

        // check for wall collisions and bounce back
        for (auto &b : local_bodies)
        {
            wallBounce(b, WIDTH_FULL, HEIGHT);
        }

        // Only necessary for graphical rendering
#ifdef GRAPHICS
        RenderScene(local_bodies, X_OFFSET);
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

void ReceiveBodies(int rank, int n_proc, Body recv_buffer[2], std::vector<Body> &foreign_bodies)
{
    MPI_Status msg_status;
    int source = (rank + 1) % n_proc;
    MPI_Probe(source, 1, MPI_COMM_WORLD, &msg_status);
    size_t msg_size = msg_status._ucount;
    size_t num_bodies = msg_size / sizeof(Body);
    //cout << "msg_size = " << msg_size << endl;

    MPI_Recv(recv_buffer, msg_size, MPI_BYTE, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    foreign_bodies.clear();
    //cout << "Rank " << rank << " received " << num_bodies << " bodies " << endl;
    for (size_t i = 0; i < num_bodies; i++)
    {
        //cout << "Rank " << rank << " received body " << recv_buffer[i] << endl;
        foreign_bodies.push_back(recv_buffer[i]);
    }
}

void RemoveBody(std::vector<Body> &local_bodies, Body &b)
{
    for (vector<Body>::iterator it = local_bodies.begin(); it != local_bodies.end(); it++)
    {
        if (it->id == b.id)
        {
            local_bodies.erase(it);
            break;
        }
    }
}

void SetupBodies(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies)
{
    {

        int N = 20;
        vector<Body> bodies;
        std::mt19937_64 rng;
        std::uniform_real_distribution<float> x_dist(5, 95);
        std::uniform_real_distribution<float> y_dist(5, 45);
        std::uniform_real_distribution<float> v_dist(-2, 2);
        std::uniform_real_distribution<float> colour_dist(0, 1);
        for(int i = 0; i < N; i++)
        {
            Body b;
            b.position = {x_dist(rng), y_dist(rng), 0};
            b.velocity = {v_dist(rng), v_dist(rng), 0};
            b.colour = {colour_dist(rng), colour_dist(rng), colour_dist(rng)};

            bool overlap = false;
            for(auto &bp : bodies)
            {
                if(checkCollision(b, bp))
                {
                    overlap = true;
                    break;
                }
            }
            if (!overlap) bodies.push_back(b);
        }
        
        //Body b1, b2, b3, b4;
        //b1.position = {30, 25, 0}; b1.velocity = {1, 0, 0}; b1.colour = {0,0,255};
        //b2.position = {90, 25, 0}; b2.velocity = {-2, 0, 0}; b2.colour = {255,0,0};
        //b3.position = {30, 10, 0}; b3.velocity = {1, 1, 0}; b3.colour = {0,255,255};
        //b4.position = {70, 40, 0}; b4.velocity = {-1, -1, 0}; b4.colour = {0,0,0};
        //vector<Body> bodies = {b1, b2, b3, b4};
        // initial partitioning
        for (auto &b : bodies)
        {
            if (b.position[0] >= X_OFFSET && b.position[0] < X_OFFSET + WIDTH)
            {
                local_bodies.push_back(b);
            }
        }
        for(auto &b : local_bodies)
        {
            std::cout << b << std::endl;
        }
    }
}
