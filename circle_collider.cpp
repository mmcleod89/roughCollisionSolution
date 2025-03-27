#include "VectorUtils.hpp"
#include <iostream>
#include <vector>
#include <optional>
#include "circle_collider.hpp"
#include <optional>

using namespace std;

unsigned int Body::current_id = 0;

Body::Body()
{
    id = current_id++;
}

bool checkCollision(const Body &b1, const Body &b2)
{
    if(mag(b1.position - b2.position) <= 2.0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void reverse(Body &b1, Body &b2)
{
    vec3 contact = (b1.position + b2.position)*0.5;

    vec3 b1_to_contact = norm(contact - b1.position);
    vec3 b2_to_contact = norm(contact - b2.position);
    vec3 v1_parallel = dot(b1.velocity, b1_to_contact) * b1_to_contact;
    vec3 v1_perp = b1.velocity - v1_parallel;
    vec3 v2_parallel = dot(b2.velocity, b2_to_contact) * b2_to_contact;
    vec3 v2_perp = b2.velocity - v2_parallel;

    // transform to symmetric frame
    vec3 delta_v = (v1_parallel + v2_parallel) * (0.5);
    vec3 v_symm = (v1_parallel - v2_parallel) * 0.5; 

    vec3 new_v1_parallel = delta_v - v_symm;
    vec3 new_v2_parallel = delta_v + v_symm;

    b1.velocity = v1_perp + new_v1_parallel;
    b2.velocity = v2_perp + new_v2_parallel;
}

void wallBounce(Body &b, unsigned int WIDTH, unsigned int HEIGHT)
{
    const float w = float(WIDTH);
    const float h = float(HEIGHT);

    if(b.position[0] < 0 || b.position[0] > w)
    {
        b.velocity[0] = -b.velocity[0];
    } 
    if(b.position[1] < 0 || b.position[1] > h)
    {
        b.velocity[1] = -b.velocity[1];
    } 
}

ostream &operator<<(ostream &os, const Body &b)
{
    os << "{" << b.id << ": " << b.position << ", " << b.velocity << "}";
    return os;
}