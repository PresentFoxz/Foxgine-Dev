#include "fox_3D.h"

#define EPSILON 1
Vec2i vert_to_screen(Vec3f v, float focal, float nearPlane) {
    Vec2i out;

    float z = v.z;
    if (z < nearPlane + EPSILON) z = nearPlane + EPSILON;

    float x = (v.x * focal) / z;
    float y = (v.y * focal) / z;

    out.x = (int)((x + 1.0f) * (SCREEN_W * 0.5f));
    out.y = (int)((1.0f - y) * (SCREEN_H * 0.5f));

    return out;
}

float dot(float x, float y, float z, float mx, float my, float mz) { return x * mx + y * my + z * mz; }

void rotateVertex(Vec3f v, Mat3x3 *m, Vec3f* out) {
    out->x = dot(v.x, v.y, v.z, m->x[0][0], m->x[0][1], m->x[0][2]);
    out->y = dot(v.x, v.y, v.z, m->x[1][0], m->x[1][1], m->x[1][2]);
    out->z = dot(v.x, v.y, v.z, m->x[2][0], m->x[2][1], m->x[2][2]);
}

void rotateVertexInPlace(Vec3f* v, Vec3f camPos, Mat3x3* m) {
    float x = v->x - camPos.x;
    float y = v->y - camPos.y;
    float z = v->z - camPos.z;

    v->x = dot(x, y, z, m->x[0][0], m->x[1][0], m->x[2][0]);
    v->y = dot(x, y, z, m->x[0][1], m->x[1][1], m->x[2][1]);
    v->z = dot(x, y, z, m->x[0][2], m->x[1][2], m->x[2][2]);
}

Vec3f lerpVertex(Vec3f a, Vec3f b, float t) {
    Vec3f r;

    r.x = a.x + (b.x - a.x) * t;
    r.y = a.y + (b.y - a.y) * t;
    r.z = a.z + (b.z - a.z) * t;

    return r;
}

int TriangleClipping(Vec3f verts[3], Triangle_t* outTri1, Triangle_t* outTri2, float nearPlane, float farPlane) {
    int inScreen[3], outScreen[3];
    int inAmt = 0, outAmt = 0;

    for (int i = 0; i < 3; i++)  {
        if (verts[i].z >= nearPlane && verts[i].z <= farPlane) { inScreen[inAmt++] = i; }
        else { outScreen[outAmt++] = i; }
    }

    Vec3f cross0, cross1;

    if (inAmt == 0) return 0;
    if (inAmt == 3) {
        *outTri1 = (Triangle_t){ verts[0], verts[1], verts[2] };
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

        cross0.z = plane0;
        cross1.z = plane1;

        *outTri1 = (Triangle_t) { verts[in0], cross0, cross1 };

        return 1;
    }

    if (inAmt == 2) {
        float plane = (verts[out0].z < nearPlane) ? nearPlane : farPlane;

        float t0 = (plane - verts[out0].z) / (verts[in0].z - verts[out0].z);
        float t1 = (plane - verts[out0].z) / (verts[in1].z - verts[out0].z);

        cross0 = lerpVertex(verts[out0], verts[in0], t0);
        cross1 = lerpVertex(verts[out0], verts[in1], t1);

        cross0.z = plane;
        cross1.z = plane;

        *outTri1 = (Triangle_t) { verts[in0], verts[in1], cross0 };
        *outTri2 = (Triangle_t) { verts[in1], cross1, cross0 };

        return 2;
    }

    return 0;
}

void computeCamMatrix(Mat3x3 *out, float x, float y, float z) {
    Mat3x3 temp;
    computeRotMatrix(&temp, x, y, z);
    for(int i = 0; i < 3; i++) { for(int j = 0; j < 3; j++) { out->x[i][j] = temp.x[j][i]; } }
}

void computeRotMatrix(Mat3x3 *out, float angleX, float angleY, float angleZ) {
    float sinX = sinf(angleX), cosX = cosf(angleX);
    float sinY = sinf(angleY), cosY = cosf(angleY);
    float sinZ = sinf(angleZ), cosZ = cosf(angleZ);

    out->x[0][0] = cosY * cosZ;
    out->x[0][1] = -cosY * sinZ;
    out->x[0][2] = sinY;

    out->x[1][0] = sinX * sinY * cosZ + cosX * sinZ;
    out->x[1][1] = -sinX * sinY * sinZ + cosX * cosZ;
    out->x[1][2] = -sinX * cosY;

    out->x[2][0] = -cosX * sinY * cosZ + sinX * sinZ;
    out->x[2][1] = cosX * sinY * sinZ + sinX * cosZ;
    out->x[2][2] = cosX * cosY;
}

void computeRotScaleMatrix(Mat3x3 *out, float angleX, float angleY, float angleZ, float sx, float sy, float sz) {
    float sinX = sinf(angleX), cosX = cosf(angleX);
    float sinY = sinf(angleY), cosY = cosf(angleY);
    float sinZ = sinf(angleZ), cosZ = cosf(angleZ);

    out->x[0][0] = (cosY * cosZ) * sx;
    out->x[0][1] = (-cosY * sinZ) * sx;
    out->x[0][2] = sinY * sx;

    out->x[1][0] = (sinX * sinY * cosZ + cosX * sinZ) * sy;
    out->x[1][1] = (-sinX * sinY * sinZ + cosX * cosZ) * sy;
    out->x[1][2] = (-sinX * cosY) * sy;

    out->x[2][0] = (-cosX * sinY * cosZ + sinX * sinZ) * sz;
    out->x[2][1] = (cosX * sinY * sinZ + sinX * cosZ) * sz;
    out->x[2][2] = (cosX * cosY) * sz;
}