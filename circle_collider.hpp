#pragma once

#include "VectorUtils.hpp"
#include <iostream>
#include <vector>
#include <optional>

using namespace std;
using namespace VecUtils;

class Body
{
public:
    Body();
    vec3 position;
    vec3 velocity;
    vec3 colour;
    unsigned int id;
private:
    static double radius;
    static unsigned int current_id;
};

ostream &operator<<(ostream &os, const Body &b);

bool checkCollision(const Body &b1, const Body &b2);

void reverse(Body &b1, Body &b2);
void wallBounce(Body &b, unsigned int W, unsigned int H);