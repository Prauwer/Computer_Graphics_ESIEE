#ifndef MAT4_H
#define MAT4_H

#include <cmath>
#include <vector>
#include <stdexcept>

struct vec3 {
    float x, y, z;

    vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    vec3 operator-(const vec3& other) const {
        return vec3(x - other.x, y - other.y, z - other.z);
    }

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    vec3 normalized() const {
        float l = length();
        if (l > 1e-6f) { // Epsilon check to avoid division by zero
            return vec3(x / l, y / l, z / l);
        }
        return vec3(0.0f, 0.0f, 0.0f);
    }

    static float dot(const vec3& a, const vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static vec3 cross(const vec3& a, const vec3& b) {
        return vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
}; 

class mat4 {
public:
    float m[16]; // Column-major order: m[col*4 + row]

    // Default constructor (identity matrix)
    mat4() {
        identity();
    }

    // Constructor from an array of 16 floats
    mat4(const float* arr) {
        for (int i = 0; i < 16; ++i) {
            m[i] = arr[i];
        }
    }

    void identity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    // Matrix multiplication: this * other
    mat4 operator*(const mat4& other) const {
        mat4 result;
        for (int i = 0; i < 4; ++i) { // column of result
            for (int j = 0; j < 4; ++j) { // row of result
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    // result.m[i*4+j] = this.m[k*4+j] * other.m[i*4+k]
                    sum += m[k * 4 + j] * other.m[i * 4 + k];
                }
                result.m[i * 4 + j] = sum;
            }
        }
        return result;
    }

    static mat4 translate(float tx, float ty, float tz) {
        mat4 result;
        result.m[12] = tx;
        result.m[13] = ty;
        result.m[14] = tz;
        return result;
    }

    static mat4 scale(float sx, float sy, float sz) {
        mat4 result;
        result.m[0] = sx;
        result.m[5] = sy;
        result.m[10] = sz;
        return result;
    }

    // Rotation around X axis
    static mat4 rotateX(float angleRadians) {
        mat4 result;
        float c = cos(angleRadians);
        float s = sin(angleRadians);
        result.m[5] = c;
        result.m[6] = s;
        result.m[9] = -s;
        result.m[10] = c;
        return result;
    }

    // Rotation around Y axis
    static mat4 rotateY(float angleRadians) {
        mat4 result;
        float c = cos(angleRadians);
        float s = sin(angleRadians);
        result.m[0] = c;
        result.m[2] = -s; // Note: some conventions might have s here
        result.m[8] = s;  // and -s here. This is one common way.
        result.m[10] = c;
        return result;
    }

    // Rotation around Z axis
    static mat4 rotateZ(float angleRadians) {
        mat4 result;
        float c = cos(angleRadians);
        float s = sin(angleRadians);
        result.m[0] = c;
        result.m[1] = s;
        result.m[4] = -s;
        result.m[5] = c;
        return result;
    }
    
    // Perspective projection matrix
    static mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
        mat4 result;
        for(int i=0; i<16; ++i) result.m[i] = 0.0f;
        float tanHalfFovy = tan(fovYRadians / 2.0f);
        result.m[0] = 1.0f / (aspect * tanHalfFovy);
        result.m[5] = 1.0f / (tanHalfFovy);
        result.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return result;
    }

    static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
        vec3 f = (center - eye).normalized();
        vec3 s = vec3::cross(f, up).normalized();
        vec3 u = vec3::cross(s, f);

        mat4 result;

        result.m[0] = s.x;
        result.m[4] = s.y;
        result.m[8] = s.z;

        result.m[1] = u.x;
        result.m[5] = u.y;
        result.m[9] = u.z;

        result.m[2] = -f.x;
        result.m[6] = -f.y;
        result.m[10] = -f.z;

        result.m[12] = -vec3::dot(s, eye);
        result.m[13] = -vec3::dot(u, eye);
        result.m[14] = vec3::dot(f, eye);
        
        return result;
    }

    const float* getPtr() const {
        return m;
    }
};

#endif // MAT4_H
