#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include "bvh.h"
#include "raster.hpp"
#include "vector.h"
#include "matrix.h"
#include "ray.hpp"
#include "camera.h"
#include "grid.hpp"
#include "bilinear.hpp"
#include "intersection.hpp"
#include "primitive.hpp"
#include "config.h"
#include "sample.h"
#include "image_sampler.h"
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING

#define GAMMA 2.2
#define SPP 64
#define XRES 1280
#define YRES 720
#define ASPECT (((float)(YRES))/XRES)
#define RAND ((float)rand()/(float)RAND_MAX)
#define RANDC (((float)rand()/(float)RAND_MAX) - 0.5)
#define IMAGE_CHANNELS 3
#define NUM_RAND_PATCHES 1000

#define GAUSS_WIDTH 2.0 / 4
float gaussian(float x, float y)
{
    float xf = expf(-x * x / (2 * GAUSS_WIDTH * GAUSS_WIDTH));
    float yf = expf(-y * y / (2 * GAUSS_WIDTH * GAUSS_WIDTH));
    return xf * yf;
}

float mitchell_1d(float x, float C) {
    float B = 1.0 - (2*C);
    x = fabsf(1.f * x);
    if(x > 2.0)
        return 0.0;
    if (x > 1.f)
        return ((-B - 6*C) * x*x*x + (6*B + 30*C) * x*x +
        (-12*B - 48*C) * x + (8*B + 24*C)) * (1.f/6.f);
    else
        return ((12 - 9*B - 6*C) * x*x*x +
        (-18 + 12*B + 6*C) * x*x +
        (6 - 2*B)) * (1.f/6.f);
}

float mitchell_2d(float x, float y, float C) {
    return mitchell_1d(x, C) * mitchell_1d(y, C);
}



int main(int argc, char **argv)
{
    /*
     ******************************************************
     * Build scene
     ******************************************************
     */
    // BVH containing our scene objects
    BVH scene;
    
    
    /*
    std::vector<Primitive *> objects(1);
    
    float yar = 1000000.0;
    float yar2 = -0.5;
    float yar3 = 150.0;
    
    // Patch 1, back wall
    objects[0] = new Bilinear(1);
    ((Bilinear *)(objects[0]))->add_time_sample(0,
                          Vec3(yar, yar, yar3),
                          Vec3(-yar, yar, yar3),
                          Vec3(-yar, -yar, yar3),
                          Vec3(yar, -yar, yar3)
                          );
    
    // Patch 2, floor
    objects[1] = new Bilinear(1);
    ((Bilinear *)(objects[1]))->add_time_sample(0,
                          Vec3(yar, yar2, yar),
                          Vec3(-yar, yar2, yar),
                          Vec3(-yar, yar2, -yar),
                          Vec3(yar, yar2, -yar)
                          );
    
    scene.add_primitives(objects);
    */
    
    
    std::vector<Primitive *> rand_objects(NUM_RAND_PATCHES);
    
    // Random patches
    std::cout << "Generating random patches..."; std::cout.flush();
    for(int i=0; i < NUM_RAND_PATCHES; i++)
    {
        float x, y, z;
        z = 15 + (RAND * NUM_RAND_PATCHES / 4);
        float s = z / 15;
        x = RANDC * 40;
        y = RANDC * 20;
        
        // Motion?
        int ms = 1;
        if(RAND < 0.25)
            ms = 2;
        
        // Flipped?
        bool flip = false;
        if(RAND < 0.5)
            flip = true;
            
        rand_objects[i] = new Bilinear(ms);
        
        for(int j = 0; j < ms; j++)
        {
            x += (RANDC * j * 8) / s;
            y += (RANDC * j * 8) / s;
            z += (RANDC * j * 8) / s;
            
            if(flip)
            {
                ((Bilinear *)(rand_objects[i]))->add_time_sample(j,
                                                 Vec3((x*s)+2, (y*s)+2, z+(RANDC*2)),
                                                 Vec3((x*s)+2, (y*s)-2, z+(RANDC*2)),
                                                 Vec3((x*s)-2, (y*s)-2, z+(RANDC*2)),
                                                 Vec3((x*s)-2, (y*s)+2, z+(RANDC*2)));
            }
            else
            {
                ((Bilinear *)(rand_objects[i]))->add_time_sample(j,
                                                 Vec3((x*s)+2, (y*s)+2, z+(RANDC*2)),
                                                 Vec3((x*s)-2, (y*s)+2, z+(RANDC*2)),
                                                 Vec3((x*s)-2, (y*s)-2, z+(RANDC*2)),
                                                 Vec3((x*s)+2, (y*s)-2, z+(RANDC*2)));

            }
        }
    }
    std::cout << " done." << std::endl; std::cout.flush();
    
    
    scene.add_primitives(rand_objects);
    
    std::cout << "Building acceleration structure... "; std::cout.flush();
    scene.finalize();
    std::cout << " done." << std::endl;  std::cout.flush();
    //return 0;
    
    // Create camera
    std::vector<Matrix44> cam_mats;
    cam_mats.resize(4);
    
    float angle = 15 * (3.14159 / 180.0);
    Vec3 axis(0.0, 0.0, 1.0);
    
    cam_mats[0].translate(Vec3(0.0, 0.0, -40.0));
    cam_mats[0].rotate(0.0, axis);
    cam_mats[0].translate(Vec3(0.0, 0.0, 20.0));
    
    cam_mats[1].translate(Vec3(0.0, 0.0, -40.0));
    cam_mats[1].rotate((angle/3)*1, axis);
    cam_mats[1].translate(Vec3(0.0, 0.0, 20.0));
    
    cam_mats[2].translate(Vec3(0.0, 0.0, -40.0));
    cam_mats[2].rotate((angle/3)*2, axis);
    cam_mats[2].translate(Vec3(0.0, 0.0, 20.0));
    
    cam_mats[3].translate(Vec3(0.0, 0.0, -40.0));
    cam_mats[3].rotate((angle/3)*3, axis);
    cam_mats[3].translate(Vec3(0.0, 0.0, 20.0));
    
    #define LENS_DIAM 1.0
    #define FOCUS_DISTANCE 40.0
    #define FOV 55
    Camera cam(cam_mats, (3.14159/180.0)*FOV, LENS_DIAM, FOCUS_DISTANCE);
    
    
    
    /*
     ******************************************************
     * Generate image
     ******************************************************
     */
    
    Raster *image = new Raster(XRES, YRES, IMAGE_CHANNELS, -1.0, -ASPECT, 1.0, ASPECT);
    Raster *image_contrib = new Raster(XRES, YRES, 1, -1.0, -ASPECT, 1.0, ASPECT);
    ImageSampler image_sampler(SPP, XRES, YRES, 2.0);
    Sample samp;
    
    Ray ray;
    Intersection inter;
    
    float x, y;
    int i;
    while(image_sampler.get_next_sample(&samp))
    {
        float rx = (samp.x - 0.5) * (image->max_x - image->min_x);
        float ry = (0.5 - samp.y) * (image->max_y - image->min_y);
        float dx = (image->max_x - image->min_x) / (2.0 * XRES);
        float dy = (image->max_y - image->min_y) / (2.0 * YRES);
        
        ray = cam.generate_ray(rx, ry, dx, dy, samp.t, samp.u, samp.v);
        ray.finalize();
        
        x = (samp.x * image->width) - 0.5;
        y = (samp.y * image->height) - 0.5;
        
        bool hit = false;
        hit = scene.intersect_ray(ray, &inter);
        for(int j=-2; j <= 2; j++)
        {
            for(int k=-2; k <= 2; k++)
            {
                int a = x + j;
                int b = y + k;
                if(a < 0 || !(a < image->width) || b < 0 || !(b < image->height))
                    continue;
                float contrib = mitchell_2d(a-x, b-y, 0.5);
                i = (image->width * b) + a;
                
                image_contrib->pixels[i] += contrib;
                if(contrib == 0)
                    continue;
                
                if(hit)
                {
                    image->pixels[i*IMAGE_CHANNELS] += inter.col.spectrum[0] * contrib;
                    image->pixels[(i*IMAGE_CHANNELS)+1] += inter.col.spectrum[1] * contrib;
                    image->pixels[(i*IMAGE_CHANNELS)+2] += inter.col.spectrum[2] * contrib;
                }
            }
        }

        
        // Print percentage complete
        static int last_perc = -1;
        int perc = image_sampler.percentage() * 100;
        if(perc > last_perc)
        {
            std::cout << perc << "%" << std::endl;
            last_perc = perc;
        }
    }
    
    std::cout << "Splits during rendering: " << Config::split_count << std::endl;
    std::cout << "Micropolygons generated during rendering: " << Config::upoly_gen_count << std::endl;
    std::cout << "Grid cache misses during rendering: " << Config::cache_misses << std::endl;
    
    // Gamma correction + dithering(256)
    for(y = 0; y < image->height; y++) {
        for(x = 0; x < image->width; x++) {
            i = x + (y*image->width);
            
            image->pixels[i*IMAGE_CHANNELS] /= image_contrib->pixels[i];
            image->pixels[i*IMAGE_CHANNELS] = std::max(image->pixels[i*IMAGE_CHANNELS], 0.0f);
            image->pixels[(i*IMAGE_CHANNELS)] = pow(image->pixels[(i*IMAGE_CHANNELS)], 1.0/GAMMA);
            image->pixels[(i*IMAGE_CHANNELS)] += RANDC / 256;
            image->pixels[(i*IMAGE_CHANNELS)] = image->pixels[(i*IMAGE_CHANNELS)] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)];
            
            image->pixels[i*IMAGE_CHANNELS+1] /= image_contrib->pixels[i];
            image->pixels[i*IMAGE_CHANNELS+1] = std::max(image->pixels[i*IMAGE_CHANNELS+1], 0.0f);
            image->pixels[(i*IMAGE_CHANNELS)+1] = pow(image->pixels[(i*IMAGE_CHANNELS)+1], 1.0/GAMMA);
            image->pixels[(i*IMAGE_CHANNELS)+1] += RANDC / 256;
            image->pixels[(i*IMAGE_CHANNELS)+1] = image->pixels[(i*IMAGE_CHANNELS)+1] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)+1];
            
            image->pixels[i*IMAGE_CHANNELS+2] /= image_contrib->pixels[i];
            image->pixels[i*IMAGE_CHANNELS+2] = std::max(image->pixels[i*IMAGE_CHANNELS+2], 0.0f);
            image->pixels[(i*IMAGE_CHANNELS)+2] = pow(image->pixels[(i*IMAGE_CHANNELS)+2], 1.0/GAMMA);
            image->pixels[(i*IMAGE_CHANNELS)+2] += RANDC / 256;
            image->pixels[(i*IMAGE_CHANNELS)+2] = image->pixels[(i*IMAGE_CHANNELS)+2] < 0.0 ? 0.0 : image->pixels[(i*IMAGE_CHANNELS)+2];
        }
    }
    
    delete image_contrib;
    
    // Save image
    ImageOutput *out = ImageOutput::create(".png");
    if(!out)
        return -1;
    
    ImageSpec spec(image->width, image->height, IMAGE_CHANNELS, TypeDesc::UINT8);
    
    out->open("test.png", spec);
    out->write_image(TypeDesc::FLOAT, image->pixels);
    out->close();
    
    // Cleanup
    delete out;
    delete image;
    
    std::cout << std::endl << "Struct sizes:" << std::endl;
    std::cout << "BBounds: " << sizeof(BBounds) << std::endl;
    std::cout << "BBox: " << sizeof(BBox) << std::endl;
    std::cout << "BVHNode: " << sizeof(BVHNode) << std::endl;
    std::cout << "Grid: " << sizeof(Grid) << std::endl;
    std::cout << "GridBVHNode: " << sizeof(GridBVHNode) << std::endl;
    std::cout << "Primitive *: " << sizeof(Primitive *) << std::endl;
    std::cout << "TimeBox<int>: " << sizeof(std::vector<int>) << std::endl;
    std::cout << "std::vector<int>: " << sizeof(std::vector<int>) << std::endl;
    
    return 0;
}
