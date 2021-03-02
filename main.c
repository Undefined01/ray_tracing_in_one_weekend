#include <math.h>
#include <stdio.h>

typedef struct {
    float d[3];
} vec3;

#define decl_vec3_op(name, op)                                 \
    vec3 vec3_s##name(const vec3 a, const float b) {           \
        vec3 c;                                                \
        for (int i = 0; i < 3; i++) c.d[i] = a.d[i] op b;      \
        return c;                                              \
    }                                                          \
    vec3 vec3_##name(const vec3 a, const vec3 b) {             \
        vec3 c;                                                \
        for (int i = 0; i < 3; i++) c.d[i] = a.d[i] op b.d[i]; \
        return c;                                              \
    }
decl_vec3_op(add, +);
decl_vec3_op(sub, -);
decl_vec3_op(mul, *);
decl_vec3_op(div, /);
float vec3_dot(const vec3 a, const vec3 b) {
    return a.d[0] * b.d[0] + a.d[1] * b.d[1] + a.d[2] * b.d[2];
}
float vec3_len(const vec3 v) { return sqrt(vec3_dot(v, v)); }
vec3 vec3_unit(const vec3 v) { return vec3_sdiv(v, vec3_len(v)); }

typedef struct {
    vec3 pos, ori;
} ray;

vec3 color(ray *r) {
    vec3 ori = vec3_unit(r->ori);
    float t = 0.5 * ori.d[1];
    // 生成上蓝下白的渐变
    return vec3_add(vec3_smul((vec3){{1.0, 1.0, 1.0}}, 0.5 - t),
                    vec3_smul((vec3){{0.5, 0.7, 1.0}}, 0.5 + t));
}

int main() {
    int nx = 200;
    int ny = 100;
    printf("P3\n%d %d\n255\n", nx, ny);

    vec3 origin = {{0, 0, 0}};
    for (int j = ny - 1; j >= 0; j--) {
        for (int i = 0; i < nx; i++) {
            float u = (float)i / (float)nx - 0.5;
            float v = (float)j / (float)ny - 0.5;

            ray r = {origin, {{u * 4, v * 2, -1.0}}};
            vec3 col = color(&r);

            int ir = (int)(255.99 * col.d[0]);
            int ig = (int)(255.99 * col.d[1]);
            int ib = (255.99 * col.d[2]);
            printf("%d %d %d\n", ir, ig, ib);
        }
    }
}
