#ifndef FOX_3D_H
#define FOX_3D_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fox_library.h"

static Vec2i vert_to_screen(Vec3f vert, float focal, float nearPlane) {
    Vec2i screen = {-999, -999};

    if (vert.z < nearPlane) vert.z = nearPlane;

    float aspect = (float)SCREEN_W / SCREEN_H;

    float x = (vert.x * focal / aspect) / vert.z;
    float y = (vert.y * focal) / vert.z;

    screen.x = (int)((x + 1.0f) * 0.5f * SCREEN_W);
    screen.y = (int)((1.0f - y) * 0.5f * SCREEN_H);

    return screen;
}

static void rotateVertexInPlace(Vec3f* v, Vec3f camPos, float camMatrix[3][3]) {
    float x = (v->x - camPos.x); float y = (v->y - camPos.y); float z = (v->z - camPos.z);

    v->x = x * camMatrix[0][0] + y * camMatrix[1][0] + z * camMatrix[2][0];
    v->y = x * camMatrix[0][1] + y * camMatrix[1][1] + z * camMatrix[2][1];
    v->z = x * camMatrix[0][2] + y * camMatrix[1][2] + z * camMatrix[2][2];
}

static void rotateVertex(Vec3f verts, float rotMat[3][3], Vec3f* vertsOut) {
    vertsOut->x = verts.x * rotMat[0][0] + verts.y * rotMat[0][1] + verts.z * rotMat[0][2];
    vertsOut->y = verts.x * rotMat[1][0] + verts.y * rotMat[1][1] + verts.z * rotMat[1][2];
    vertsOut->z = verts.x * rotMat[2][0] + verts.y * rotMat[2][1] + verts.z * rotMat[2][2];
}

static void computeCamMatrix(float m[3][3], float pitchX, float yawY, float rollZ) {
    float sinY = sinf(yawY),   cosY = cosf(yawY);
    float sinX = sinf(pitchX), cosX = cosf(pitchX);
    float sinZ = sinf(rollZ),  cosZ = cosf(rollZ);
    
    m[0][0] = cosY * cosZ + sinY * sinX * sinZ;
    m[0][1] = -cosY * sinZ + sinY * sinX * cosZ;
    m[0][2] = sinY * cosX;

    m[1][0] = cosX * sinZ;
    m[1][1] = cosX * cosZ;
    m[1][2] = -sinX;

    m[2][0] = -sinY * cosZ + cosY * sinX * sinZ;
    m[2][1] = sinY * sinZ + cosY * sinX * cosZ;
    m[2][2] = cosY * cosX;
}

static void computeRotScaleMatrix(float rotMat[3][3], float angleX, float angleY, float angleZ, float sx, float sy, float sz) {
    float sinX = sinf(angleX), cosX = cosf(angleX);
    float sinY = sinf(angleY), cosY = cosf(angleY);
    float sinZ = sinf(angleZ), cosZ = cosf(angleZ);

    rotMat[0][0] = (cosY * cosZ) * sx;
    rotMat[0][1] = (-cosY * sinZ) * sx;
    rotMat[0][2] = (sinY) * sx;

    rotMat[1][0] = (sinX * sinY * cosZ + cosX * sinZ) * sy;
    rotMat[1][1] = (-sinX * sinY * sinZ + cosX * cosZ) * sy;
    rotMat[1][2] = (-sinX * cosY) * sy;

    rotMat[2][0] = (-cosX * sinY * cosZ + sinX * sinZ) * sz;
    rotMat[2][1] = (cosX * sinY * sinZ + sinX * cosZ) * sz;
    rotMat[2][2] = (cosX * cosY) * sz;
}

static Vec3f lerpVertex(Vec3f a, Vec3f b, float t) {
    Vec3f r;

    r.x = a.x + t * (b.x - a.x);
    r.y = a.y + t * (b.y - a.y);
    r.z = a.z + t * (b.z - a.z);

    return r;
}

int TriangleClipping(Vec3f verts[3], Triangle_t* outTri1, Triangle_t* outTri2, float nearPlane, float farPlane) {
    int inScreen[3], outScreen[3];
    int inAmt = 0, outAmt = 0;

    for (int i = 0; i < 3; i++) {
        if (verts[i].z >= nearPlane && verts[i].z <= farPlane) {
            inScreen[inAmt++] = i;
        } else {
            outScreen[outAmt++] = i;
        }
    }

    Vec3f cross0, cross1;

    if (inAmt == 0) return 0;

    if (inAmt == 3) {
        *outTri1 = (Triangle_t){verts[0], verts[1], verts[2]};
        return 1;
    }

    int in0 = inScreen[0];
    int in1 = inScreen[1];
    int out0 = outScreen[0];
    int out1 = outScreen[1];

    if (inAmt == 1) {
        float plane0 = (verts[out0].z < nearPlane) ? nearPlane : farPlane;
        float plane1 = (verts[out1].z < nearPlane) ? nearPlane : farPlane;

        float t0 = (plane0 - verts[out0].z) / (verts[in0].z - verts[out0].z);
        float t1 = (plane1 - verts[out1].z) / (verts[in0].z - verts[out1].z);

        cross0 = lerpVertex(verts[out0], verts[in0], t0);
        cross1 = lerpVertex(verts[out1], verts[in0], t1);

        *outTri1 = (Triangle_t){verts[in0], cross0, cross1};
        return 1;
    }
    
    if (inAmt == 2) {
        float plane = (verts[out0].z < nearPlane) ? nearPlane : farPlane;

        float t0 = (plane - verts[out0].z) / (verts[in0].z - verts[out0].z);
        float t1 = (plane - verts[out0].z) / (verts[in1].z - verts[out0].z);

        cross0 = lerpVertex(verts[out0], verts[in0], t0);
        cross1 = lerpVertex(verts[out0], verts[in1], t1);

        *outTri1 = (Triangle_t){verts[in0], verts[in1], cross0};
        *outTri2 = (Triangle_t){verts[in1], cross1, cross0};
        return 2;
    }

    return 0;
}

#endif