#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include "circle_collider.hpp"

void RenderScene(std::vector<Body> &bodies, unsigned int x_offset=0);

void SetupRendering(int rank, unsigned int w, unsigned int h);

void CheckForEvents(bool &keep_going, int n_proc, int rank);

void CheckForTerminationSignal(int n_proc, int rank, bool &keep_going);