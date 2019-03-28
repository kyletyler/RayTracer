#ifndef RAYH
#define RAYH
#include "vec3.h"

// A ray is essentially a parametric line: 
//	p(t) = A + t*B
// p is 3D position along a line in 3D
// A is the ray origin
// B is the ray direction
// t is a real number
// different t's will move the point along the ray
// if t >= 0, can go anywhere on the 3D line (parts in front of A)
// This is called a RAY!!!

class ray {
public:
	ray() {}
	// A ray is defined by starting point (A) and ray direction (B)
	ray(const vec3& a, const vec3& b) { A = a; B = b; }
	vec3 origin() const    { return A; }
	vec3 direction() const { return B; }

	// Defining the ray equation
	vec3 point_at_parameter(float t) const { return A + t * B; }

	vec3 A;
	vec3 B;
};

#endif