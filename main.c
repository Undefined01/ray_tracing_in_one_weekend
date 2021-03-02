#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// 生成[0, 1)的随机数
float rand_float() { return (float)rand() / RAND_MAX; }

typedef struct {
    float d[3];
} Vec3;

#define decl_vec3_op(name, op)                                 \
    Vec3 vec3_s##name(const Vec3 a, const float b) {           \
        Vec3 c;                                                \
        for (int i = 0; i < 3; i++) c.d[i] = a.d[i] op b;      \
        return c;                                              \
    }                                                          \
    Vec3 vec3_##name(const Vec3 a, const Vec3 b) {             \
        Vec3 c;                                                \
        for (int i = 0; i < 3; i++) c.d[i] = a.d[i] op b.d[i]; \
        return c;                                              \
    }
decl_vec3_op(add, +);
decl_vec3_op(sub, -);
decl_vec3_op(mul, *);
decl_vec3_op(div, /);
float vec3_dot(const Vec3 a, const Vec3 b) {
    return a.d[0] * b.d[0] + a.d[1] * b.d[1] + a.d[2] * b.d[2];
}
float vec3_len(const Vec3 v) { return sqrt(vec3_dot(v, v)); }
Vec3 vec3_unit(const Vec3 v) { return vec3_sdiv(v, vec3_len(v)); }

// 生成单位球内的随机向量
Vec3 rand_vec3() {
    Vec3 res;
    do
        res = (Vec3){
            {rand_float() * 2 - 1, rand_float() * 2 - 1, rand_float() * 2 - 1}};
    while (vec3_dot(res, res) > 1);
    return res;
}

typedef struct {
    // 起始位置
    Vec3 pos;
    // 方向，单位向量
    Vec3 ori;
} Ray;

// 声明而不定义
typedef struct Object Object;
typedef struct Material Material;
typedef struct HitInfo HitInfo;

// 判断某物体是否与光线碰撞。如果碰撞，通过 HitInfo 指针返回碰撞点信息
typedef bool (*IsHitFunc)(const Object *this, const Ray *r, HitInfo *info);

// 光与物体碰撞后计算散射结果的函数，与材质有关。
// attenuation为衰减系数，scattered为散射光线
typedef bool (*ScatterFunc)(const HitInfo *info, const Ray *r,
                            Vec3 *attenuation, Ray *scattered);

// 物体的抽象类
typedef struct Object {
    IsHitFunc is_hit;
} Object;

// 材质的抽象类
typedef struct Material {
    ScatterFunc scatter;
} Material;

// 光线与物体碰撞时的信息
typedef struct HitInfo {
    // 位置参数，实际坐标为 ray.pos + t * ray.ori
    float t;
    // 位置坐标，与上面计算结果相等
    Vec3 pos;
    // 碰撞点法线，单位向量
    Vec3 norm;
    // 碰撞点材质渲染器
    Material *material;
} HitInfo;

// 漫反射
typedef struct {
    // 通过指针的方式实现接口
    Material material;
    Vec3 attenuation;
} Lambertian;

bool lambertian_scatter(const HitInfo *info,
                        const Ray *r __attribute__((__unused__)),
                        Vec3 *attenuation, Ray *scattered) {
    Lambertian *self = (Lambertian *)(info->material);
    *attenuation = self->attenuation;
    // 漫反射
    *scattered = (Ray){info->pos, vec3_unit(vec3_add(info->norm, rand_vec3()))};
    return true;
}

// 金属反射
typedef struct {
    // 通过指针的方式实现接口
    Material material;
    Vec3 attenuation;
    // 金属的漫反射
    float fuzz;
} Metal;

bool metal_scatter(const HitInfo *info,
                   const Ray *r __attribute__((__unused__)), Vec3 *attenuation,
                   Ray *scattered) {
    Metal *self = (Metal *)(info->material);
    *attenuation = self->attenuation;
    // 反射光线（b = a - 2 * a * n * n， n为法向单位向量）
    *scattered = (Ray){
        info->pos,
        vec3_sub(
            r->ori,
            vec3_smul(vec3_smul(info->norm, vec3_dot(r->ori, info->norm)), 2))};
    // 漫反射
    scattered->ori =
        vec3_unit(vec3_add(scattered->ori, vec3_smul(rand_vec3(), self->fuzz)));
    return true;
}

typedef struct {
    // 通过指针的方式实现接口
    Object parent;
    Material *mat;
    Vec3 pos;
    float r;
} Sphere;

bool sphere_is_hit(const Object *this, const Ray *r, HitInfo *info) {
    const Sphere *s = (const Sphere *)this;
    Vec3 oc = vec3_sub(r->pos, s->pos);
    float a = vec3_dot(r->ori, r->ori);
    float b2 = vec3_dot(oc, r->ori);  // 该变量实际上为 2 * b
    float c = vec3_dot(oc, oc) - s->r * s->r;
    float delta = b2 * b2 - a * c;
    if (delta <= FLT_EPSILON) return false;
    float t = (-b2 - sqrt(delta)) / a;
    if (t <= FLT_EPSILON) return false;
    info->t = t;
    info->pos = vec3_add(r->pos, vec3_smul(r->ori, t));
    info->norm = vec3_sdiv(vec3_sub(info->pos, s->pos), s->r);
    info->material = s->mat;
    return true;
}

// 渲染的世界，可视为将世界内所有物体组合称了一个实体
typedef struct {
    // 通过指针的方式实现接口
    Object parent;
    // 世界内的所有物体
    Object **obj;
    size_t len;
} World;

bool world_is_hit(const Object *this, const Ray *r, HitInfo *info) {
    const World *self = (const World *)this;
    bool is_hit = false;
    info->t = FLT_MAX;
    HitInfo tmp_info;
    for (size_t i = 0; i < self->len; i++) {
        if (self->obj[i]->is_hit(self->obj[i], r, &tmp_info)) {
            is_hit = true;
            if (info->t > tmp_info.t) *info = tmp_info;
        }
    }
    return is_hit;
}

Vec3 render(const Ray *r, const World *world) {
    HitInfo info;
    // 判断是否与世界中的物体碰撞
    if (((Object *)world)->is_hit((Object *)world, r, &info)) {
        Vec3 attenuation;
        Ray scattered;
        if (info.material->scatter(&info, r, &attenuation, &scattered))
            return vec3_mul(render(&scattered, world), attenuation);
    }
    // 生成上蓝下白的渐变作为背景
    Vec3 ori = vec3_unit(r->ori);
    float t = 0.5 * ori.d[1];
    return vec3_add(vec3_smul((Vec3){{1.0, 1.0, 1.0}}, 0.5 - t),
                    vec3_smul((Vec3){{0.5, 0.7, 1.0}}, 0.5 + t));
}

typedef struct Camera {
    Vec3 pos;
    Vec3 ori;
} Camera;

// 根据摄像机自身的位置和相对视角 (u,v) ，给出需要渲染的光线
Ray camera_get_ray(const Camera *self, const float u, const float v) {
    return (Ray){self->pos, vec3_unit(vec3_add(self->ori, (Vec3){{u, v, 0}}))};
}

int main() {
    int nx = 200;
    int ny = 100;
    // 单位长度的像素个数
    int nu = 100;
    // 像素内重复取样次数
    int ns = 100;
    printf("P3\n%d %d\n255\n", nx, ny);

    Camera camera = {{{0, 0, 0}}, {{0, 0, -0.5}}};
    Lambertian m1 = {{lambertian_scatter}, {{0.8, 0.3, 0.3}}};
    Lambertian m2 = {{lambertian_scatter}, {{0.8, 0.8, 0.0}}};
    Sphere s1 = {{sphere_is_hit}, (Material *)&m1, {{0, 0, -1}}, 0.5};
    Sphere s2 = {{sphere_is_hit}, (Material *)&m2, {{0, -100.5, -1}}, 100};
    Metal m3 = {{metal_scatter}, {{0.8, 0.6, 0.2}}, 0.3};
    Metal m4 = {{metal_scatter}, {{0.8, 0.8, 0.8}}, 1.0};
    Sphere s3 = {{sphere_is_hit}, (Material *)&m3, {{1, 0, -1}}, 0.2};
    Sphere s4 = {{sphere_is_hit}, (Material *)&m4, {{-1, 0, -1}}, 0.5};
    Object *obj_list[] = {(Object *)&s1, (Object *)&s2, (Object *)&s3,
                          (Object *)&s4};
    World world = {
        {world_is_hit}, obj_list, sizeof(obj_list) / sizeof(Object *)};
    for (int j = ny - 1; j >= 0; j--) {
        for (int i = 0; i < nx; i++) {
            Vec3 pixel = {{0, 0, 0}};
            float u = ((float)i - (float)nx / 2) / nu;
            float v = ((float)j - (float)ny / 2) / nu;
            for (int s = 0; s < ns; s++) {
                Ray r = camera_get_ray(&camera, u + rand_float() / nu,
                                       v + rand_float() / nu);
                pixel = vec3_add(pixel, render(&r, &world));
            }
            pixel = vec3_sdiv(pixel, ns);

            if (!(0 <= pixel.d[0] && pixel.d[0] <= 1 && 0 <= pixel.d[1] &&
                  pixel.d[1] <= 1 && 0 <= pixel.d[2] && pixel.d[2] <= 1)) {
                fprintf(stderr, "Color out of bound at (%d,%d) (%f,%f,%f)\n", i,
                        j, pixel.d[0], pixel.d[1], pixel.d[2]);
            }
            int ir = (int)(255.99 * pixel.d[0]);
            int ig = (int)(255.99 * pixel.d[1]);
            int ib = (255.99 * pixel.d[2]);
            printf("%d %d %d\n", ir, ig, ib);
        }
    }
}
