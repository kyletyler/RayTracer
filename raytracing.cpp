#include "pch.h"
#include <fstream>
#include <iostream>
#include <random>
#include <stdlib.h>
#include "sphere.h"
#include "hitablelist.h"
#include "float.h"
#include "camera.h"
#include "material.h"

// TODO: 
//		 implement parallelism
//	     try intel denoising thing
//		 lights


// Sending a ray to pixel, determining what color the hit pixel should be
vec3 color(const ray& r, hitable *world, int depth) {
	hit_record rec;

	if (world->hit(r, 0.001, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
		
		// scatter and attenuate it from the surface
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			// keep tracing the scatter ray recursively
			return attenuation * color(scattered, world, depth + 1);
		}
		// ray would be absorbed
		else {
			return vec3(0, 0, 0);
		}
	}
	// ray hits sky
	else {
		vec3 unit_direction = unit_vector(r.direction()); // make -1 < y < 1
		
		// scale unit_direction to 0 < t < 1
		float t = 0.5*(unit_direction.y() + 1.0); 

		// Linear blend (lerp) between two things:
		//	  blended_value = (1-t)*start_value + t*end_value
		return (1.0 - t)*vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
	}
}

hitable *random_scene() {
	int n = 500;
	hitable **list = new hitable*[n + 1];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(0.5, 0.5, 0.5))); // ground sphere
	int i = 1;

	int numSpheres = 5; // the number of spheres will be this to the second power

	for (int a = -numSpheres; a < numSpheres; a++) {
		for (int b = -numSpheres; b < numSpheres; b++) {
			float choose_mat = ((double)rand() / RAND_MAX);
			vec3 center(a*((double)rand() / RAND_MAX), 2*((double)rand() / RAND_MAX) + .2, b*((double)rand() / RAND_MAX));

			if ((center - vec3(4, 0.2, 0)).length() > 0.9) {
				if (choose_mat < 0.8) { // diffuse
					list[i++] = new sphere(center, 0.2*((double)rand() / RAND_MAX), new lambertian(vec3(((double)rand() / RAND_MAX)*((double)rand() / RAND_MAX), ((double)rand() / RAND_MAX)*((double)rand() / RAND_MAX), ((double)rand() / RAND_MAX)*((double)rand() / RAND_MAX))));
				}
				else if (choose_mat < 0.95) { // metal
					list[i++] = new sphere(center, 0.2*((double)rand() / RAND_MAX),
						new metal(vec3(0.5*(1 + ((double)rand() / RAND_MAX)), 0.5*(1 + ((double)rand() / RAND_MAX)), 0.5*(1 + ((double)rand() / RAND_MAX))), 0.5*((double)rand() / RAND_MAX)));
				}
				else { // glass
					list[i++] = new sphere(center, 0.2*((double)rand() / RAND_MAX), new dielectric(1.5));
				}
			}
		}
	}

	// these ones aren't random
	list[i++] = new sphere(vec3(-2.5, 1, 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.7)));
	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
	list[i++] = new sphere(vec3(2.5, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

	return new hitable_list(list, i);
}

int main() {
	int imageScale = 6;
	int nx = 200 * imageScale; // num pixels width
	int ny = 100 * imageScale;  // num pixels height
	int ns = 10; // num samples

	std::ofstream myfile;
	myfile.open("ex.ppm");

	// P3 means colors are in ASCII, then nx columns and ny rows
	// 255 for max color, then we can define RGB triplets
	myfile << "P3\n" << nx << " " << ny << "\n255\n";
	
	// generate a bunch of random spheres with random materials
	hitable *world = new hitable_list();
	world = random_scene(); 

	// set camera view values
	vec3 lookfrom(1, 2, 11);
	vec3 lookat(0, 1, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.1f;

	camera cam(lookfrom, lookat, vec3(0, 1, 0), 20, float(nx) / float(ny), aperture, dist_to_focus);
	
	// Writing image to the plain text ppm file
	// Pixels are written out in rows with pixels left to right

	for (int j = ny - 1; j >= 0; j--) { // writing to each row from top to bottom
		std::cout << j << std::endl;
		for (int i = 0; i < nx; i++) {  // writing to each column from left to right
			
			vec3 col(0, 0, 0); // vec3 representing the color for current pixel
			
			// For a given pixel we have several samples within that pixel and send rays through each of the samples
			// The colors of these rays are then averaged :
			for (int s = 0; s < ns; s++) {
				float u = float(i + ((double) rand() / RAND_MAX)) / float(nx);
				float v = float(j + ((double) rand() / RAND_MAX)) / float(ny);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0);
				col += color(r, world, 0);
			}
			col /= float(ns);
			col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			int ir = int(255.99*col[0]);
			int ig = int(255.99*col[1]);
			int ib = int(255.99*col[2]);

			// write the RGB color triplet to file
			myfile << ir << " " << ig << " " << ib << "\n";
		}
	}

	std::cout << "exited for loop..." << std::endl;

	myfile.close();
	return 0;
}
