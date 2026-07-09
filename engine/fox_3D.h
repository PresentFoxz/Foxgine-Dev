#ifndef FOX_3D_H
#define FOX_3D_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "fox_library.h"
#include "fox_fixed.h"

static inline Vec2i vert_to_screen(Vec3s24 v, qFixed16_t focal, qFixed24x8_t nearPlane) {
    Vec2i out;

    if(v.z <= nearPlane) {
        out.x = -999;
        out.y = -999;
        return out;
    }

    qFixed16_t xp = fixed24_to_16(v.x);
    qFixed16_t yp = fixed24_to_16(v.y);
    qFixed16_t zp = fixed24_to_16(v.z);

    qFixed16_t x = project_div(xp, focal, zp);
    qFixed16_t y = project_div(yp, focal, zp);

    int64_t sx = ((int64_t)x + FIXED_ONE16) * SCREEN_W;
    int64_t sy = ((int64_t)FIXED_ONE16 - y) * SCREEN_H;

    out.x = (int)(sx >> 17);
    out.y = (int)(sy >> 17);

    return out;
}

static inline qFixed24x8_t dot24_16(qFixed24x8_t x, qFixed24x8_t y, qFixed24x8_t z, qFixed16_t mx, qFixed16_t my, qFixed16_t mz) {
    int64_t acc = (int64_t)x * mx + (int64_t)y * my + (int64_t)z * mz;
    return (qFixed24x8_t)(acc >> 16);
}

static inline void rotateVertex(Vec3s24 v, Mat3x3 *m, Vec3s24* out) {
    out->x = dot24_16(v.x, v.y, v.z, m->x[0][0], m->x[0][1], m->x[0][2]);
    out->y = dot24_16(v.x, v.y, v.z, m->x[1][0], m->x[1][1], m->x[1][2]);
    out->z = dot24_16(v.x, v.y, v.z, m->x[2][0], m->x[2][1], m->x[2][2]);
}

static inline void rotateVertexInPlace(Vec3s24* v, Vec3s24 camPos, Mat3x3* m) {
    qFixed24x8_t x = v->x - camPos.x;
    qFixed24x8_t y = v->y - camPos.y;
    qFixed24x8_t z = v->z - camPos.z;

    v->x = dot24_16(x, y, z, m->x[0][0], m->x[1][0], m->x[2][0]);
    v->y = dot24_16(x, y, z, m->x[0][1], m->x[1][1], m->x[2][1]);
    v->z = dot24_16(x, y, z, m->x[0][2], m->x[1][2], m->x[2][2]);
}

static inline void computeRotScaleMatrix(Mat3x3 *out, qFixed24x8_t angleX, qFixed24x8_t angleY, qFixed24x8_t angleZ, qFixed24x8_t sx, qFixed24x8_t sy, qFixed24x8_t sz) {
    qFixed16_t sinX = fsin(rad24_to_index(angleX)), cosX = fcos(rad24_to_index(angleX));
    qFixed16_t sinY = fsin(rad24_to_index(angleY)), cosY = fcos(rad24_to_index(angleY));
    qFixed16_t sinZ = fsin(rad24_to_index(angleZ)), cosZ = fcos(rad24_to_index(angleZ));

    qFixed16_t xs = fixed24_to_16(sx);
    qFixed16_t ys = fixed24_to_16(sy);
    qFixed16_t zs = fixed24_to_16(sz);

    out->x[0][0] = mul_16(mul_16(cosY, cosZ), xs);
    out->x[0][1] = mul_16(mul_16(-cosY, sinZ), xs);
    out->x[0][2] = mul_16(sinY, xs);

    out->x[1][0] = mul_16(mul_16(sinX, mul_16(sinY, cosZ)) + mul_16(cosX, sinZ), ys);
    out->x[1][1] = mul_16(mul_16(-sinX, mul_16(sinY, sinZ)) + mul_16(cosX, cosZ), ys);
    out->x[1][2] = mul_16(mul_16(-sinX, cosY), ys);

    out->x[2][0] = mul_16(mul_16(-cosX, mul_16(sinY, cosZ)) + mul_16(sinX, sinZ), zs);
    out->x[2][1] = mul_16(mul_16(cosX, mul_16(sinY, sinZ)) + mul_16(sinX, cosZ), zs);
    out->x[2][2] = mul_16(mul_16(cosX, cosY), zs);
}

static inline void computeCamMatrix(Mat3x3 *out, qFixed24x8_t x, qFixed24x8_t y, qFixed24x8_t z) {
    Mat3x3 temp;
    computeRotScaleMatrix(&temp, x, y, z, to_fixed24(1.0f), to_fixed24(1.0f), to_fixed24(1.0f));
    for(int i = 0; i < 3; i++) { for(int j = 0; j < 3; j++) { out->x[i][j] = temp.x[j][i]; } }
}

static inline Vec3s24 lerpVertex(Vec3s24 a, Vec3s24 b, qFixed24x8_t t) {
    Vec3s24 r;

    r.x = mul_24(mul_24(a.x + t, (b.x - a.x)), to_fixed24(1.0f));
    r.y = mul_24(mul_24(a.y + t, (b.y - a.y)), to_fixed24(1.0f));
    r.z = mul_24(mul_24(a.z + t, (b.z - a.z)), to_fixed24(1.0f));

    return r;
}

static inline int TriangleClipping(Vec3s24 verts[3], Triangle_t* outTri1, Triangle_t* outTri2, qFixed24x8_t nearPlane, qFixed24x8_t farPlane) {
    int inScreen[3], outScreen[3];
    int inAmt = 0, outAmt = 0;

    for (int i = 0; i < 3; i++) {
        if (verts[i].z >= nearPlane && verts[i].z <= farPlane) {
            inScreen[inAmt++] = i;
        } else {
            outScreen[outAmt++] = i;
        }
    }

    Vec3s24 cross0, cross1;

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
        qFixed24x8_t plane0 = (verts[out0].z < nearPlane) ? nearPlane : farPlane;
        qFixed24x8_t plane1 = (verts[out1].z < nearPlane) ? nearPlane : farPlane;

        qFixed24x8_t t0 = div_24(plane0 - verts[out0].z, verts[in0].z - verts[out0].z);
        qFixed24x8_t t1 = div_24(plane1 - verts[out1].z, verts[in0].z - verts[out1].z);

        cross0 = lerpVertex(verts[out0], verts[in0], t0);
        cross1 = lerpVertex(verts[out1], verts[in0], t1);

        *outTri1 = (Triangle_t){ .p0 = verts[in0], .p1 = cross0, .p2 = cross1};
        return 1;
    }
    
    if (inAmt == 2) {
        qFixed24x8_t plane = (verts[out0].z < nearPlane) ? nearPlane : farPlane;

        qFixed24x8_t t0 = div_24(plane - verts[out0].z, verts[in0].z - verts[out0].z);
        qFixed24x8_t t1 = div_24(plane - verts[out0].z, verts[in1].z - verts[out0].z);

        cross0 = lerpVertex(verts[out0], verts[in0], t0);
        cross1 = lerpVertex(verts[out0], verts[in1], t1);

        *outTri1 = (Triangle_t){ .p0 = verts[in0], .p1 = verts[in1], .p2 = cross0 };
        *outTri2 = (Triangle_t){ .p0 = verts[in1], .p1 = cross1, .p2 = cross0 };
        return 2;
    }

    return 0;
}

#endif