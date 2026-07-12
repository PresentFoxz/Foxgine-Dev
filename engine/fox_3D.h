#ifndef FOX_3D_H
#define FOX_3D_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fox_library.h"
#include "fox_mesh.h"

#ifdef PLATFORM_GBA

ARM_IWRAM int TriangleClipping(Vec3s24 verts[3], Triangle_t* outTri1, Triangle_t* outTri2, qFixed24x8_t nearPlane, qFixed24x8_t farPlane);
ARM_IWRAM Vec2i vert_to_screen(Vec3s24 v, qFixed16_t focal, qFixed24x8_t nearPlane);
ARM_IWRAM void rotateVertex(Vec3s24 v, Mat3x3 *m, Vec3s24* out);
ARM_IWRAM void rotateVertexInPlace(Vec3s24* v, Vec3s24 camPos, Mat3x3* m);
ARM_IWRAM void computeRotScaleMatrix(Mat3x3 *out, qFixed24x8_t angleX, qFixed24x8_t angleY, qFixed24x8_t angleZ, qFixed24x8_t sx, qFixed24x8_t sy, qFixed24x8_t sz);
ARM_IWRAM void computeRotMatrix(Mat3x3 *out, qFixed24x8_t angleX, qFixed24x8_t angleY, qFixed24x8_t angleZ);
ARM_IWRAM void computeCamMatrix(Mat3x3 *out, qFixed24x8_t x, qFixed24x8_t y, qFixed24x8_t z);

#else

int TriangleClipping(Vec3s24 verts[3], Triangle_t* outTri1, Triangle_t* outTri2, qFixed24x8_t nearPlane, qFixed24x8_t farPlane);
Vec2i vert_to_screen(Vec3s24 v, qFixed16_t focal, qFixed24x8_t nearPlane);
void rotateVertex(Vec3s24 v, Mat3x3 *m, Vec3s24* out);
void rotateVertexInPlace(Vec3s24* v, Vec3s24 camPos, Mat3x3* m);
void computeRotScaleMatrix(Mat3x3 *out, qFixed24x8_t angleX, qFixed24x8_t angleY, qFixed24x8_t angleZ, qFixed24x8_t sx, qFixed24x8_t sy, qFixed24x8_t sz);
void computeRotMatrix(Mat3x3 *out, qFixed24x8_t angleX, qFixed24x8_t angleY, qFixed24x8_t angleZ);
void computeCamMatrix(Mat3x3 *out, qFixed24x8_t x, qFixed24x8_t y, qFixed24x8_t z);

#endif

qFixed24x8_t dot24_16(qFixed24x8_t x, qFixed24x8_t y, qFixed24x8_t z, qFixed16_t mx, qFixed16_t my, qFixed16_t mz);
Vec3s24 lerpVertex(Vec3s24 a, Vec3s24 b, qFixed24x8_t t);

#endif