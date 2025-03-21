#pragma once
void SetupBodies(const unsigned int X_OFFSET, const unsigned int WIDTH, std::vector<Body> &local_bodies);

void RemoveBody(std::vector<Body> &local_bodies, Body &b);

void ReceiveBodies(int rank, int n_proc, Body recv_buffer[2], std::vector<Body> &foreign_bodies);
