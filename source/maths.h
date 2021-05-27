#pragma once

#include <cmath>

struct float2 {
    float x;
    float y;
};

struct float3 {
    float x;
    float y;
    float z;
};

struct float4x4 {
    float data[4][4];
};

struct aabbox2 {
    float2 center;
    float2 extent;
};

struct aabbox3 {
    float3 center;
    float3 extent;
};

struct plane {
    float3 normal;
    float distance;
};

struct frustum {
    plane data[6];
};

inline float dot(const float3& lhs, const float3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

inline float3 sub(const float3& lhs, const float3& rhs) {
    return float3{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

inline float3 cross(const float3& lhs, const float3& rhs) {
    return float3{ lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x };
}

inline float length(const float3& value) {
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

inline float3 normalize(const float3& value) {
    float multiplier = 1.f / length(value);
    return float3{ value.x * multiplier, value.y * multiplier, value.z * multiplier };
}

inline float4x4 look_at(const float3& source, const float3& target, const float3& up) {
    float3 f(normalize(sub(source, target)));
    float3 s(normalize(cross(up, f)));
    float3 u(cross(f, s));

    return float4x4{
        s.x,             u.x,             f.x,             0.f,
        s.y,             u.y,             f.y,             0.f,
        s.z,             u.z,             f.z,             0.f,
        -dot(source, s), -dot(source, u), -dot(source, f), 1.f
    };
}

inline float4x4 perspective(float fov_y, float aspect, float z_near, float z_far) {
    float tan_half_fov_y = std::tan(fov_y * 0.5f);
    return float4x4{
        1.f / (aspect * tan_half_fov_y), 0.f,                  0.f,                                0.f,
        0.f,                             1.f / tan_half_fov_y, 0.f,                                0.f,
        0.f,                             0.f,                  z_far / (z_near - z_far),          -1.f,
        0.f,                             0.f,                  z_far * z_near / (z_near - z_far),  0.f
    };
}

inline float4x4 mul(const float4x4& lhs, const float4x4& rhs) {
    return float4x4{
        lhs.data[0][0] * rhs.data[0][0] + lhs.data[0][1] * rhs.data[1][0] + lhs.data[0][2] * rhs.data[2][0] + lhs.data[0][3] * rhs.data[3][0],
        lhs.data[0][0] * rhs.data[0][1] + lhs.data[0][1] * rhs.data[1][1] + lhs.data[0][2] * rhs.data[2][1] + lhs.data[0][3] * rhs.data[3][1],
        lhs.data[0][0] * rhs.data[0][2] + lhs.data[0][1] * rhs.data[1][2] + lhs.data[0][2] * rhs.data[2][2] + lhs.data[0][3] * rhs.data[3][2],
        lhs.data[0][0] * rhs.data[0][3] + lhs.data[0][1] * rhs.data[1][3] + lhs.data[0][2] * rhs.data[2][3] + lhs.data[0][3] * rhs.data[3][3],
        lhs.data[1][0] * rhs.data[0][0] + lhs.data[1][1] * rhs.data[1][0] + lhs.data[1][2] * rhs.data[2][0] + lhs.data[1][3] * rhs.data[3][0],
        lhs.data[1][0] * rhs.data[0][1] + lhs.data[1][1] * rhs.data[1][1] + lhs.data[1][2] * rhs.data[2][1] + lhs.data[1][3] * rhs.data[3][1],
        lhs.data[1][0] * rhs.data[0][2] + lhs.data[1][1] * rhs.data[1][2] + lhs.data[1][2] * rhs.data[2][2] + lhs.data[1][3] * rhs.data[3][2],
        lhs.data[1][0] * rhs.data[0][3] + lhs.data[1][1] * rhs.data[1][3] + lhs.data[1][2] * rhs.data[2][3] + lhs.data[1][3] * rhs.data[3][3],
        lhs.data[2][0] * rhs.data[0][0] + lhs.data[2][1] * rhs.data[1][0] + lhs.data[2][2] * rhs.data[2][0] + lhs.data[2][3] * rhs.data[3][0],
        lhs.data[2][0] * rhs.data[0][1] + lhs.data[2][1] * rhs.data[1][1] + lhs.data[2][2] * rhs.data[2][1] + lhs.data[2][3] * rhs.data[3][1],
        lhs.data[2][0] * rhs.data[0][2] + lhs.data[2][1] * rhs.data[1][2] + lhs.data[2][2] * rhs.data[2][2] + lhs.data[2][3] * rhs.data[3][2],
        lhs.data[2][0] * rhs.data[0][3] + lhs.data[2][1] * rhs.data[1][3] + lhs.data[2][2] * rhs.data[2][3] + lhs.data[2][3] * rhs.data[3][3],
        lhs.data[3][0] * rhs.data[0][0] + lhs.data[3][1] * rhs.data[1][0] + lhs.data[3][2] * rhs.data[2][0] + lhs.data[3][3] * rhs.data[3][0],
        lhs.data[3][0] * rhs.data[0][1] + lhs.data[3][1] * rhs.data[1][1] + lhs.data[3][2] * rhs.data[2][1] + lhs.data[3][3] * rhs.data[3][1],
        lhs.data[3][0] * rhs.data[0][2] + lhs.data[3][1] * rhs.data[1][2] + lhs.data[3][2] * rhs.data[2][2] + lhs.data[3][3] * rhs.data[3][2],
        lhs.data[3][0] * rhs.data[0][3] + lhs.data[3][1] * rhs.data[1][3] + lhs.data[3][2] * rhs.data[2][3] + lhs.data[3][3] * rhs.data[3][3]
    };
}

inline plane normalize(const plane& value) {
    float multiplier = 1.f / length(value.normal);
    return plane{ float3{ value.normal.x * multiplier, value.normal.y * multiplier, value.normal.z * multiplier }, value.distance * multiplier };
}

inline frustum frustum_from_float4x4(const float4x4& view_projection_matrix) {
    return frustum{
        normalize(plane{
            float3{
                view_projection_matrix.data[0][3] + view_projection_matrix.data[0][0],
                view_projection_matrix.data[1][3] + view_projection_matrix.data[1][0],
                view_projection_matrix.data[2][3] + view_projection_matrix.data[2][0]
            },
            view_projection_matrix.data[3][3] + view_projection_matrix.data[3][0]
        }),
        normalize(plane{
            float3{
                view_projection_matrix.data[0][3] - view_projection_matrix.data[0][0],
                view_projection_matrix.data[1][3] - view_projection_matrix.data[1][0],
                view_projection_matrix.data[2][3] - view_projection_matrix.data[2][0]
            },
            view_projection_matrix.data[3][3] - view_projection_matrix.data[3][0]
        }),
        normalize(plane{
            float3{
                view_projection_matrix.data[0][3] + view_projection_matrix.data[0][1],
                view_projection_matrix.data[1][3] + view_projection_matrix.data[1][1],
                view_projection_matrix.data[2][3] + view_projection_matrix.data[2][1]
            },
            view_projection_matrix.data[3][3] + view_projection_matrix.data[3][1]
        }),
        normalize(plane{
            float3{
                view_projection_matrix.data[0][3] - view_projection_matrix.data[0][1],
                view_projection_matrix.data[1][3] - view_projection_matrix.data[1][1],
                view_projection_matrix.data[2][3] - view_projection_matrix.data[2][1]
            },
            view_projection_matrix.data[3][3] - view_projection_matrix.data[3][1]
        }),
        normalize(plane{
            float3{
                view_projection_matrix.data[0][2],
                view_projection_matrix.data[1][2],
                view_projection_matrix.data[2][2]
            },
            view_projection_matrix.data[3][2]
        }),
        normalize(plane{
            float3{
                view_projection_matrix.data[0][3] - view_projection_matrix.data[0][2],
                view_projection_matrix.data[1][3] - view_projection_matrix.data[1][2],
                view_projection_matrix.data[2][3] - view_projection_matrix.data[2][2]
            },
            view_projection_matrix.data[3][3] - view_projection_matrix.data[3][2]
        })
    };
}

inline bool intersect(const aabbox2& lhs, const aabbox3& rhs) {
    return std::abs(lhs.center.x - rhs.center.x) <= lhs.extent.x + rhs.extent.x &&
           std::abs(lhs.center.y - rhs.center.z) <= lhs.extent.y + rhs.extent.z;
}

inline bool intersect(const aabbox3& lhs, const aabbox3& rhs) {
    return std::abs(lhs.center.x - rhs.center.x) <= lhs.extent.x + rhs.extent.x &&
           std::abs(lhs.center.y - rhs.center.y) <= lhs.extent.y + rhs.extent.y &&
           std::abs(lhs.center.z - rhs.center.z) <= lhs.extent.z + rhs.extent.z;
}

inline bool intersect(const aabbox3& lhs, const frustum& rhs) {
    for (const plane& plane : rhs.data) {
        float3 abs_normal{ std::abs(plane.normal.x), std::abs(plane.normal.y), std::abs(plane.normal.z) };
        if (dot(lhs.center, plane.normal) + plane.distance + dot(lhs.extent, abs_normal) < 0.f) {
            return false;
        }
    }
    return true;
}
