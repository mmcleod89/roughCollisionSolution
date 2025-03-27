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
#include "MpiTags.h"

int main()
{
    MPI_Init(NULL, NULL);
    int rank, n_proc;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n_proc);

    const unsigned int WIDTH_FULL = 100;
    const unsigned int HALF_WIDTH = WIDTH_FULL / n_proc;
    const unsigned int HEIGHT = 50;
    const unsigned int X_OFFSET = HALF_WIDTH * rank;
    const unsigned int X_BUFFER = HALF_WIDTH + (rank==0 ? -2*Body::radius : 2*Body::radius);

    printf("Rank %d has buffer pos %d\n", rank, X_BUFFER);

    #ifdef GRAPHICS
    SetupRendering(rank, HALF_WIDTH, HEIGHT);
    #endif

    vector<Body> local_bodies;  // bodies in this parition
    vector<Body> transfer_list;  // bodies to send to neighbour

    // Estimate the necessary size of receive buffer and allocate it
    double buffer_area = 4*Body::radius * HEIGHT;
    int recv_buffer_size = static_cast<int>(buffer_area / (M_PI*Body::radius*Body::radius));
    if(rank==0) printf("Expected maximum size of buffer is %d\n", recv_buffer_size);
    Body *recv_buffer = new Body[recv_buffer_size]; // expected maximum size of receive buffer

    RandomSetup(X_OFFSET, HALF_WIDTH, local_bodies);
    //TrivialSetup(X_OFFSET, HALF_WIDTH, local_bodies);
    

    bool keep_going = true;
    double t = 0;
    double t_max = 200;
    unsigned int steps = 0;
    double dt = 0.1;
    while (keep_going && t < t_max)
    {
        #ifdef GRAPHICS
        auto frame_start = SDL_GetTicks();
        #endif

        t = (steps++) * dt;
        // update positions
        for (auto &b : local_bodies)
        {
            b.position = b.position + b.velocity * dt;
        }

        // build list of particles beyond the buffer line
        transfer_list.clear();
        for (auto &b : local_bodies)
        {
            if(BeyondBoundary(b, rank, X_BUFFER))
            {
                transfer_list.push_back(b);
            }
        }

        // communicate the buffer lists; ring communication to break deadlock
        int num_bodies_received = 0;
        if(rank == 0)
        {
            MPI_Ssend(transfer_list.data(), transfer_list.size() * sizeof(Body), MPI_BYTE, 1, MessageTags::bufferZone, MPI_COMM_WORLD);
            num_bodies_received = ReceiveBodies(rank, n_proc, recv_buffer, recv_buffer_size);
        }
        else if(rank==1)
        {
            num_bodies_received = ReceiveBodies(rank, n_proc, recv_buffer, recv_buffer_size);
            MPI_Ssend(transfer_list.data(), transfer_list.size() * sizeof(Body), MPI_BYTE, 0, MessageTags::bufferZone, MPI_COMM_WORLD);
        }

        // adopt foreign bodies that are inside our region
        for(int i = 0; i < num_bodies_received; i++)
        {
            Body &b = recv_buffer[i];
            if(b.position[0] >= float(X_OFFSET) && b.position[0] < float(X_OFFSET + HALF_WIDTH))
            {
                //cout << "Rank " << rank << " adopted a new body " << b << endl;
                local_bodies.push_back(b);
            }
        }

        //abandon bodies that leave our region
        for(auto &b : transfer_list)
        {
            if(b.position[0] >= float(X_OFFSET+HALF_WIDTH) || b.position[0] < X_OFFSET)
            {
                //cout << "Rank " << rank << " abandoned a body " << b << endl;
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
                    //cout << "Collision on rank " << rank << endl;
                    reverse(local_bodies[i], local_bodies[j]);
                }
            }
        }

        // check for buffer collisions and bounce back
        for(auto &local : transfer_list)
        {
            for(int i = 0; i < num_bodies_received; i++)
            {
                Body& foreign = recv_buffer[i];
                if(checkCollision(local, foreign))
                {
                    cout << "Buffer collision on rank " << rank << " " << local << ", " << foreign << endl;
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
        auto frame_time = SDL_GetTicks() - frame_start;
        const unsigned int frame_min = 1000/60;
        if(frame_time < frame_min)
        {
            SDL_Delay(frame_min - frame_time);
        }
        RenderScene(local_bodies, X_OFFSET, X_BUFFER, HEIGHT);
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

    delete[] recv_buffer;

    MPI_Finalize();
    return 0;
}

int ReceiveBodies(int rank, int n_proc, Body *recv_buffer, int &max_size)
{
    MPI_Status msg_status;
    int source = (rank + 1) % n_proc;
    MPI_Probe(source, 1, MPI_COMM_WORLD, &msg_status);
    int msg_size = 0;
    MPI_Get_count(&msg_status, MPI_BYTE, &msg_size);  // this is the size in bytes! 
    int num_bodies = msg_size / sizeof(Body);  // this is the number of bodies
    
    // If it's too big we'll need to do a reallocation
    if (num_bodies > max_size)
    {
        delete[] recv_buffer;
        recv_buffer = new Body[num_bodies];
    }

    MPI_Recv(recv_buffer, msg_size, MPI_BYTE, source, MessageTags::bufferZone, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return num_bodies;
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

bool BeyondBoundary(const Body &b, const int rank, const unsigned int X_BUFFER)
{
    return (rank==0) ? b.position[0] > X_BUFFER : b.position[0] < X_BUFFER;
}

void TrivialSetup(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies)
{
    Body b;
    b.position = {(float) X_OFFSET + 25, 25, 0};
    b.velocity = {(WIDTH - b.position[0]) / 12, 0, 0};
    local_bodies.push_back(b);
}

void RandomSetup(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies)
{
    {

        int N = 5;
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
