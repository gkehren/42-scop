#pragma once

#include <algorithm>
#include <stdexcept>
#include <cmath>

struct Vec3 {
	float x, y, z;

	Vec3() : x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	Vec3 operator+(const Vec3& other) const {
		return Vec3(x + other.x, y + other.y, z + other.z);
	}

	Vec3 operator-(const Vec3& other) const {
		return Vec3(x - other.x, y - other.y, z - other.z);
	}

	Vec3 operator*(float scalar) const {
		return Vec3(x * scalar, y * scalar, z * scalar);
	}

	Vec3 operator-() const {
		return Vec3(-x, -y, -z);
	}

	Vec3 operator/(float scalar) const {
		return Vec3(x / scalar, y / scalar, z / scalar);
	}

	Vec3& operator+=(const Vec3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vec3& operator-=(const Vec3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	static float length(Vec3 v) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	static Vec3 normalize(const Vec3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length != 0.0f) {
		float invLength = 1.0f / length;
		return {v.x * invLength, v.y * invLength, v.z * invLength};
	} else {
		return {0.0f, 0.0f, 0.0f};
	}
}

	Vec3 normalize() const {
		float len = length(*this);
		return {x / len, y / len, z / len};
	}

	static Vec3 cross(Vec3 v1, Vec3 v2) {
		return {
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};
	}

	Vec3 cross(const Vec3& other) const {
		return Vec3(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
	}

	static float dot(Vec3 v1, Vec3 v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	float dot(const Vec3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	static const GLfloat* value_ptr(Vec3& vec) {
		return &vec.x;
	}

	static Vec3 min(const Vec3& a, const Vec3& b) {
		return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}

	static Vec3 max(const Vec3& a, const Vec3& b) {
		return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}
};

struct Mat3;

struct Mat4 {
	float data[16];

	Mat4() {
		for (int i = 0; i < 16; i++)
			data[i] = 0.0f;
		data[0] = data[5] = data[10] = data[15] = 1.0f;
	}

	Mat4(const float* elements) {
		for (int i = 0; i < 16; i++)
			data[i] = elements[i];
	}

	Mat4(const Mat3& mat3);

	Mat4(float x) {
		for (int i = 0; i < 16; i++)
			data[i] = 0.0f;

		data[0] = data[5] = data[10] = data[15] = x;
	}

	float& operator()(int row, int col) {
		return data[row * 4 + col];
	}

	const float& at(int row, int col) const {
		if (row >= 0 && row < 4 && col >= 0 && col < 4) {
			return data[row * 4 + col];
		} else {
			throw std::out_of_range("Mat4 index out of range");
		}
	}

	Mat4 operator*(Mat4& other) {
		Mat4 result;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				float sum = 0.0f;
				for (int k = 0; k < 4; k++) {
					sum += (*this)(i, k) * other(k, j);
				}
				result(i, j) = sum;
			}
		}

		return result;
	}

	Mat4 operator*(const Mat4& other) const {
		Mat4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.data[i * 4 + j] = 0.0f;
				for (int k = 0; k < 4; ++k) {
					result.data[i * 4 + j] += this->data[i * 4 + k] * other.data[k * 4 + j];
				}
			}
		}
		return result;
	}

	Mat4& operator+=(const Mat4& other) {
		for (int i = 0; i < 16; i++)
			data[i] += other.data[i];
		return *this;
	}

	Mat4& operator-=(const Mat4& other) {
		for (int i = 0; i < 16; i++)
			data[i] -= other.data[i];
		return *this;
	}

	static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
	{
		Vec3 zaxis = Vec3::normalize(eye - target);    // The "forward" vector.
		Vec3 xaxis = Vec3::normalize(up.cross(zaxis)); // The "right" vector.
		Vec3 yaxis = zaxis.cross(xaxis);     // The "up" vector.

		// Create a 4x4 view matrix from the right, up, forward and eye position vectors
		float elements[16] = {
			xaxis.x,            yaxis.x,            zaxis.x,       0,
			xaxis.y,            yaxis.y,            zaxis.y,       0,
			xaxis.z,            yaxis.z,            zaxis.z,       0,
			-xaxis.dot(eye),    -yaxis.dot(eye),    -zaxis.dot(eye),    1
		};

		Mat4 viewMatrix(elements);

		return viewMatrix;
	}

	static Mat4 perspective(float fovy, float aspect, float near, float far)
	{
		Mat4 result;

		float tanHalfFov = tan(fovy / 2.0f);
		float range = near - far;

		result(0, 0) = 1.0f / (tanHalfFov * aspect);
		result(1, 1) = 1.0f / tanHalfFov;
		result(2, 2) = (near + far) / range;
		result(2, 3) = -1.0f;
		result(3, 2) = 2.0f * far * near / range;

		return result;
	}

	static Mat4 rotateY(float angle) {
		Mat4 result;

		float cosTheta = cos(angle);
		float sinTheta = sin(angle);

		result(0, 0) = cosTheta;
		result(0, 2) = -sinTheta;
		result(2, 0) = sinTheta;
		result(2, 2) = cosTheta;

		return result;
	}

	static Mat4 translate(const Vec3& translation)
	{
		Mat4 result(1.0f);

		result.data[12] = translation.x;
		result.data[13] = translation.y;
		result.data[14] = translation.z;

		return result;
	}

	static const float* value_ptr(const Mat4& matrix) {
		return matrix.data;
	}
};

struct Mat3 {
	float data[9];

	Mat3() {
		for (int i = 0; i < 9; i++)
			data[i] = 0.0f;
		data[0] = data[4] = data[8] = 1.0f;
	}

	Mat3(const Mat4& mat4) {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3 ; j++)
				data[i * 3 + j] = mat4.at(i, j);
	}

	float& operator()(int row, int col) {
		return data[row * 3 + col];
	}

	const float& operator()(int row, int col) const {
		return data[row * 3 + col];
	}

	Vec3 operator*(const Vec3& vec) const {
		return Vec3(
			data[0] * vec.x + data[1] * vec.y + data[2] * vec.z,
			data[3] * vec.x + data[4] * vec.y + data[5] * vec.z,
			data[6] * vec.x + data[7] * vec.y + data[8] * vec.z
		);
	}

	Mat3 operator*(float scalar) const {
		Mat3 result;

		for (int i = 0; i < 9; i++)
			result.data[i] = data[i] * scalar;

		return result;
	}

	static Mat3 transpose(const Mat3& matrix) {
		Mat3 result;

		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3 ; j++)
				result.data[i * 3 + j] = matrix.data[j * 3 + i];

		return result;
	}
};

struct TextureCoord {
	float u, v;
};

struct Vertex
{
	Vec3			position;
	Vec3			normal;
	TextureCoord	texture;
};
