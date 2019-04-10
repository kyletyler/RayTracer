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
#include <omp.h>

inline double getRand() {
	return ((double)rand() / RAND_MAX);
}

inline double randInRange(double lower, double upper) {
	return lower + rand() / (RAND_MAX / (upper - lower + 1) + 1);
}

// Sending a ray to pixel, determining what color the hit pixel should be
vec3 color(const ray& r, hitable *world, int depth) {
	hit_record rec;

	if (world->hit(r, 0.001, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
	
		// scatter and attenuate it from the surface
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			// keep tracing the scattered ray recursively
			return attenuation * color(scattered, world, depth + 1);
		}
		// ray is absorbed
		else {
			return vec3(0., 0., 0.);
		}
	}
	// ray hits sky
	else {
		// make -1 < y < 1
		vec3 unit_direction = unit_vector(r.direction()); 
		
		// scale unit_direction to 0 < t < 1
		float t = 0.5*(unit_direction.y() + 1.); 

		// Linear blend (lerp) between two things:
		//	  blended_value = (1-t)*start_value + t*end_value
		return (1. - t)*vec3(1., 1., 1.) + t * vec3(0.5, 0.7, 1.);
	}
}

// defining the scene to be rendered
// defines a few spheres and fills out the scene with randomly generated spheres
hitable *random_scene() {
	int n = 500;
	hitable **list = new hitable*[n+1];
	list[0] = new sphere(vec3(0., -1000., 0.), 1000., new lambertian(vec3(0.5, 0.5, 0.5))); // the ground
	
	// the number of randomly placed spheres in the scene will be this to the second power
	int numSpheres = 5; 
	int i = 1;

	// random floating spheres
	for (int a = -numSpheres; a < numSpheres; a++) {
		for (int b = -numSpheres; b < numSpheres; b++) {
			float choose_mat = getRand();
			// set center of this sphere based on for loop indices (spreads the random spheres out)
			vec3 center(a*getRand(), 2.*getRand() + .2, b*getRand()); 

			if ((center - vec3(4., 0.2, 0.)).length() > 0.9) {
				// diffuse
				if (choose_mat < 0.8) { 
					list[i++] = new sphere(center, 0.2*getRand(), 
						        new lambertian(vec3(getRand()*getRand(), getRand()*getRand(), getRand()*getRand())));
				}
				// metal
				else if (choose_mat < 0.95) {
					list[i++] = new sphere(center, 0.2*getRand(),
						        new metal(vec3(0.5*(1. + getRand()), 0.5*(1. + getRand()), 0.5*(1. + getRand())), 0.5*getRand()));
				}
				// glass
				else {
					list[i++] = new sphere(center, 0.2*getRand(), new dielectric(1.5));
				}
			}
		}
	}

	// range to place stars in scene
	double starfield_lowerx = -450.;
	double starfield_upperx = 250.;
	double starfield_lowery = -50.;
	double starfield_uppery = 80.;

	// place random star spheres in the background in range
	for (int c = 0; c < 100; c++) {
		list[i++] = new sphere(vec3(randInRange(starfield_lowerx, starfield_upperx), randInRange(starfield_lowery, starfield_uppery), -1000.), 
			        5., new lambertian(vec3(1., 1., 0.)));
	}

	// some bigger "hand placed" spheres
	list[i++] = new sphere(vec3(-2.5, 1., 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.7)));
	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
	list[i++] = new sphere(vec3(2.5, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

	return new hitable_list(list, i);
}

int main() {
	// get current wall clock time
	double startTime = omp_get_wtime();

	const int IMAGESCALE = 5;     
	const int NX = 200 * IMAGESCALE; // num pixels width
	const int NY = 100 * IMAGESCALE; // num pixels height
	const int NS = 20;               // num samples

	std::ofstream myfile;
	myfile.open("ex.ppm");

	// P3 means colors are in ASCII, then nx columns and ny rows
	// 255 for max color, then we can define RGB triplets
	myfile << "P3\n" << NX << " " << NY << "\n255\n";
	
	// generate a bunch of random spheres with random materials
	hitable *world = new hitable_list();
	world = random_scene(); 

	// set camera view values
	vec3 lookfrom(1., 2., 11.);
	vec3 lookat(0., 1., 0.);
	float dist_to_focus = 10.;
	float aperture = 0.1f;

	camera cam(lookfrom, lookat, vec3(0., 1., 0.), 20., float(NX) / float(NY), aperture, dist_to_focus);

	// Writing image to the plain text ppm file
	// Pixels are written out in rows with pixels left to right
	for (int j = NY - 1; j >= 0; j--) { // writing to each row from top to bottom
		std::cout << j << std::endl;
		for (int i = 0; i < NX; i++) {  // writing to each column from left to right
			
			// vec3 representing the color for current pixel
			vec3 col(0., 0., 0.); 
			
			// For a given pixel we have several samples within that pixel and send rays through each of the samples
			// The colors of these rays are then averaged:
			for (int s = 0; s < NS; s++) {
				float u = float(i + getRand()) / float(NX);
				float v = float(j + getRand()) / float(NY);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.);
				col += color(r, world, 0.);
			}
			col /= float(NS);
			col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			int ir = int(255.99*col[0]);
			int ig = int(255.99*col[1]);
			int ib = int(255.99*col[2]);

			// write the RGB color triplet to file
			myfile << ir << " " << ig << " " << ib << "\n";
		}
	}

	myfile.close();

	double renderTime = omp_get_wtime() - startTime;
	printf("Render Time: %8.2lf seconds\n", renderTime);

	return 0;
}
