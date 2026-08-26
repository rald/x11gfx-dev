#ifndef GL3D_H
#define GL3D_H

#include "gl2d.h"
#include <math.h>

// 3D Vector Structure
typedef struct {
    float x, y, z;
} Vec3;

// 3D Triangle / Face Structure with Color
typedef struct {
    Vec3 p[3];
    unsigned long color;
    float depth; // Average Z for sorting (Painter's Algorithm)
} Triangle3D;

// Cube Structure defined by center, size, and 6 face colors
typedef struct {
    Vec3 pos;       // Center position
    Vec3 size;      // Width (x), Height (y), Depth (z)
    Vec3 rot;       // Rotation angles (pitch, yaw, roll) in radians
    unsigned long face_colors[6]; // +X, -X, +Y, -Y, +Z, -Z
} Cube3D;

// Matrix & Transformation Helpers
typedef struct {
    float m[4][4];
} Mat4;

// Function Prototypes
static inline Mat4 gl3d_mat4_identity();
static inline Mat4 gl3d_mat4_rotation(float rx, float ry, float rz);
static inline Vec3 gl3d_mat4_mul_vec3(Mat4 mat, Vec3 v);
static inline Vec3 gl3d_vec3_add(Vec3 a, Vec3 b);
static inline Vec3 gl3d_vec3_sub(Vec3 a, Vec3 b);
static inline Vec3 gl3d_vec3_scale(Vec3 v, float s);
static inline float gl3d_vec3_dot(Vec3 a, Vec3 b);
static inline Vec3 gl3d_vec3_cross(Vec3 a, Vec3 b);

// Rendering Function Prototype (Isometric)
void gl3d_draw_cube(FastCanvas *canvas, Cube3D *cube, float scale);

#ifdef GL3D_IMPLEMENTATION

static inline Mat4 gl3d_mat4_identity() {
    Mat4 mat = {{{0}}};
    for (int i = 0; i < 4; i++) mat.m[i][i] = 1.0f;
    return mat;
}

static inline Mat4 gl3d_mat4_rotation(float rx, float ry, float rz) {
    float cx = cosf(rx), sx = sinf(rx);
    float cy = cosf(ry), sy = sinf(ry);
    float cz = cosf(rz), sz = sinf(rz);

    Mat4 mx = {{{1,0,0,0}, {0,cx,-sx,0}, {0,sx,cx,0}, {0,0,0,1}}};
    Mat4 my = {{{cy,0,sy,0}, {0,1,0,0}, {-sy,0,cy,0}, {0,0,0,1}}};
    Mat4 mz = {{{cz,-sz,0,0}, {sz,cz,0,0}, {0,0,1,0}, {0,0,0,1}}};

    float temp[4][4];
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++) {
            temp[i][j] = 0;
            for(int k=0; k<4; k++) temp[i][j] += my.m[i][k] * mx.m[k][j];
        }

    Mat4 res = {{{0}}};
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++) {
            res.m[i][j] = 0;
            for(int k=0; k<4; k++) res.m[i][j] += mz.m[i][k] * temp[k][j];
        }

    return res;
}

static inline Vec3 gl3d_mat4_mul_vec3(Mat4 mat, Vec3 v) {
    Vec3 out;
    out.x = mat.m[0][0] * v.x + mat.m[0][1] * v.y + mat.m[0][2] * v.z + mat.m[0][3];
    out.y = mat.m[1][0] * v.x + mat.m[1][1] * v.y + mat.m[1][2] * v.z + mat.m[1][3];
    out.z = mat.m[2][0] * v.x + mat.m[2][1] * v.y + mat.m[2][2] * v.z + mat.m[2][3];
    return out;
}

static inline Vec3 gl3d_vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3 gl3d_vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3 gl3d_vec3_scale(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
static inline float gl3d_vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline Vec3 gl3d_vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline void gl3d_fill_triangle(FastCanvas *canvas, Vec3 v0, Vec3 v1, Vec3 v2, unsigned long color) {
    int min_x = (int)fminf(v0.x, fminf(v1.x, v2.x));
    int max_x = (int)fmaxf(v0.x, fmaxf(v1.x, v2.x));
    int min_y = (int)fminf(v0.y, fminf(v1.y, v2.y));
    int max_y = (int)fmaxf(v0.y, fmaxf(v1.y, v2.y));

    if (min_x < 0) min_x = 0;
    if (max_x >= canvas->width) max_x = canvas->width - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= canvas->height) max_y = canvas->height - 1;

    float area = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
    if (fabsf(area) < 0.01f) return;

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float w0 = ((v1.x - px) * (v2.y - py) - (v1.y - py) * (v2.x - px)) / area;
            float w1 = ((v2.x - px) * (v0.y - py) - (v2.y - py) * (v0.x - px)) / area;
            float w2 = 1.0f - w0 - w1;

            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                gl2d_pset(canvas, x, y, color);
            }
        }
    }
}

// Draw Cube Implementation with Isometric Projection
void gl3d_draw_cube(FastCanvas *canvas, Cube3D *cube, float scale) {
    float hx = cube->size.x / 2.0f;
    float hy = cube->size.y / 2.0f;
    float hz = cube->size.z / 2.0f;

    // 8 Local Vertices assigned safely at runtime
    Vec3 local_verts[8];
    local_verts[0] = (Vec3){-hx, -hy, -hz};
    local_verts[1] = (Vec3){ hx, -hy, -hz};
    local_verts[2] = (Vec3){ hx,  hy, -hz};
    local_verts[3] = (Vec3){-hx,  hy, -hz};
    local_verts[4] = (Vec3){-hx, -hy,  hz};
    local_verts[5] = (Vec3){ hx, -hy,  hz};
    local_verts[6] = (Vec3){ hx,  hy,  hz};
    local_verts[7] = (Vec3){-hx,  hy,  hz};

    Mat4 rot_mat = gl3d_mat4_rotation(cube->rot.x, cube->rot.y, cube->rot.z);
    Vec3 transformed_verts[8];

    for (int i = 0; i < 8; i++) {
        Vec3 rot = gl3d_mat4_mul_vec3(rot_mat, local_verts[i]);
        transformed_verts[i] = gl3d_vec3_add(rot, cube->pos);
    }

    int face_indices[6][4] = {
        {1, 5, 6, 2}, // +X
        {4, 0, 3, 7}, // -X
        {3, 2, 6, 7}, // +Y
        {4, 5, 1, 0}, // -Y
        {5, 4, 7, 6}, // +Z
        {0, 1, 2, 3}  // -Z
    };

    Triangle3D triangles[12];
    int tri_count = 0;

    for (int f = 0; f < 6; f++) {
        int *fi = face_indices[f];
        Vec3 p0 = transformed_verts[fi[0]];
        Vec3 p1 = transformed_verts[fi[1]];
        Vec3 p2 = transformed_verts[fi[2]];
        Vec3 p3 = transformed_verts[fi[3]];

        Vec3 edge1 = gl3d_vec3_sub(p1, p0);
        Vec3 edge2 = gl3d_vec3_sub(p2, p0);
        Vec3 normal = gl3d_vec3_cross(edge1, edge2);

        if (normal.z < 0) {
            triangles[tri_count] = (Triangle3D){{p0, p1, p2}, cube->face_colors[f], (p0.z + p1.z + p2.z) / 3.0f};
            tri_count++;
            triangles[tri_count] = (Triangle3D){{p0, p2, p3}, cube->face_colors[f], (p0.z + p2.z + p3.z) / 3.0f};
            tri_count++;
        }
    }

    for (int i = 0; i < tri_count - 1; i++) {
        for (int j = 0; j < tri_count - i - 1; j++) {
            if (triangles[j].depth < triangles[j + 1].depth) {
                Triangle3D temp = triangles[j];
                triangles[j] = triangles[j + 1];
                triangles[j + 1] = temp;
            }
        }
    }

    float cx = canvas->width / 2.0f;
    float cy = canvas->height / 2.0f;

    for (int i = 0; i < tri_count; i++) {
        Vec3 proj[3];
        for (int k = 0; k < 3; k++) {
            proj[k].x = triangles[i].p[k].x * scale + cx;
            proj[k].y = -triangles[i].p[k].y * scale + cy;
            proj[k].z = triangles[i].p[k].z;
        }
        gl3d_fill_triangle(canvas, proj[0], proj[1], proj[2], triangles[i].color);
    }
}

#endif // GL3D_IMPLEMENTATION
#endif // GL3D_H
