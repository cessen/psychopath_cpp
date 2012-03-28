#ifndef IMAGE_SAMPLER_H
#define IMAGE_SAMPLER_H

#include "sample.h"
#include <vector>

/*
 * An image sampler.  Returns samples for use by the renderer.
 * Image plane <x,y> samples are returned on the [0,1] square, + edge buffer for filtering.
 * Lens <u,v> samples are returned on the [0,1) square.
 * Time samples are returned on the [0,1) line.
 * All 1d, 2d, and 3d samples are returned on the [0,1) line, square,
 * and cube respectively.
 * The renderer is expected to transform sample ranges as necessary.
 */
class ImageSampler
{
    private:
        /* General settings. */
        unsigned int spp;  // Approximate number of samples per pixel
        unsigned int res_x, res_y;  // Image resolution in pixels
        float f_width;  // Filter width; not used directly
                        // here, but we need to know so we can buffer
                        // the image around the edges
        unsigned int bucket_size;  // Width and height of each bucket in pixels
    
        /* State information. */
        unsigned int hilbert_order, hilbert_res;
        unsigned int points_traversed;
        unsigned int x, y, s;
        
        /* For reporting percentages. */
        unsigned int samp_taken;
        unsigned int tot_samp;
        
        
    public:
        ImageSampler(int spp_,
                     int res_x_, int res_y_,
                     float f_width_=1.0,
                     int bucket_size_=0);
        ~ImageSampler();
        
        void init_tile();
        bool get_next_sample(Sample *sample);
        
        float percentage() const
        {
            return ((float)(samp_taken)) / tot_samp;
        }
};

#endif
