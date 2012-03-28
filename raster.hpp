#ifndef RASTER_HPP
#define RASTER_HPP

/*
 * A lightweight raster image buffer.
 * Includes a mapping to 2d coordinates.
 * Pixels are stored in left-to-right, top-to-bottom order, with all the
 * channels of a pixel stored next to each other.
 */
class Raster {
    public:
        int width, height; // Resolution of the image
        float min_x, min_y; // Minimum x/y coordinates of the image
        float max_x, max_y; // Maximum x/y coordinates of the image
        int channels; // Channels per pixel
        float *pixels; // Pixel data
    
        Raster(int w, int h, int cc, float x1, float y1, float x2, float y2);
        ~Raster();
};

#endif
