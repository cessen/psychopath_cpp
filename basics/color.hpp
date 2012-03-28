#ifndef COLOR_HPP
#define COLOR_HPP

#define SPECTRUM_COUNT 3

struct Color
{
        float spectrum[SPECTRUM_COUNT];

        Color(float n=0.0) {
            
            for(int i=0; i < SPECTRUM_COUNT; i++)
            {
                spectrum[i] = n;
            }
        }
        
        Color(float r_, float g_, float b_) {
            spectrum[0] = r_;
            spectrum[1] = g_;
            spectrum[2] = b_;
        }
        
        Color operator+(Color &b);
        Color operator-(Color &b);
        Color operator*(Color &b);
        Color operator*(float b);
};

#endif
