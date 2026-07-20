#ifndef FOX_3D_H
#define FOX_3D_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fox_library.h"
#include "fox_mesh.h"

int TriangleClipping(Vec3f verts[3], Triangle_t* outTri1, Triangle_t* outTri2, float nearPlane, float farPlane);
Vec2i vert_to_screen(Vec3f v, float focal, float nearPlane);
void rotateVertex(Vec3f v, Mat3x3 *m, Vec3f* out);
void rotateVertexInPlace(Vec3f* v, Vec3f camPos, Mat3x3* m);
void computeRotScaleMatrix(Mat3x3 *out, float angleX, float angleY, float angleZ, float sx, float sy, float sz);
void computeRotMatrix(Mat3x3 *out, float angleX, float angleY, float angleZ);
void computeCamMatrix(Mat3x3 *out, float x, float y, float z);
float dot24_16(float x, float y, float z, float mx, float my, float mz);
Vec3f lerpVertex(Vec3f a, Vec3f b, float t);

#endif