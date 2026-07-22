#ifndef PERLIN_H
#define PERLIN_H

void perlinInit(unsigned int seed);
float perlin2D(float x, float y);
float perlin3D(float x, float y, float z);

#endif