#pragma once

void TrivialSetup(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies);

void RandomSetup(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies);

void RemoveBody(std::vector<Body> &local_bodies, Body &b);

int ReceiveBodies(int rank, int n_proc, Body *recv_buffer, int &max_size);

bool BeyondBoundary(const Body &b, const int rank, const unsigned int X_BUFFER);