#pragma once
#include <array> 
#include <iostream> 
#include <cmath>

namespace VecUtils
{
    template<size_t n>
    using vec = std::array<float, n>;

    using vec3 = vec<3>;

    template<size_t n>
    float dot(const vec<n> &v1, const vec<n> &v2)
    {
        float dot = 0;
        for(size_t i = 0; i < n; i++)
        {
            dot += v1[i] * v2[i];
        }
        return dot;
    }

    template<size_t n>
    vec<n> operator*(vec<n> const& v, float x)
    {
        vec<n> out;
        for(size_t i = 0; i < n; i++)
        {
            out[i] = v[i] * x;
        }
        return out;
    }

    template<size_t n>
    vec<n> operator*(float x, vec<n> const& v)
    {
        return operator*(v, x);
    }

    template<size_t n>
    vec<n> operator+(vec<n> const& v1, vec<n> const &v2)
    {
        vec<n> out;
        for(size_t i = 0; i < n; i++)
        {
            out[i] = v1[i] + v2[i];
        }
        return out;
    }

    template<size_t n>
    vec<n> operator-(vec<n> const& v1, vec<n> const &v2)
    {
        vec<n> out;
        for(size_t i = 0; i < n; i++)
        {
            out[i] = v1[i] - v2[i];
        }
        return out;
    }

    template<size_t n>
    vec<n> norm(const vec<n> &v)
    {
        float magnitude = dot(v, v);
        if(magnitude == 0)
        {
            throw std::runtime_error("Vector with magnitude zero in norm");
        }
        return v * (1.0 / sqrt(magnitude));
    }

    template<size_t n>
    float mag(const vec<n> &v)
    {
        return dot(v, v);
    }

    template<size_t n>
    std::ostream& operator<<(std::ostream &os, const vec<n> &v)
    {
        os << "(";
        for(size_t i = 0; i < (n-1); i++)
        {
            os << v[i] << ", ";
        }
        os << v[n-1] << ")";
        return os;
    }
}