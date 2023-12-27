#include "../include/Scop.hpp"

Mat4::Mat4(const Mat3& mat3) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			(*this)(i, j) = mat3(i, j);

	(*this)(3, 0) = (*this)(3, 1) = (*this)(3, 2) = 0.0f;
	(*this)(0, 3) = (*this)(1, 3) = (*this)(2, 3) = 0.0f;
	(*this)(3, 3) = 1.0f;
}

Mat3 operator*(float scalar, const Mat3& matrix) {
	return matrix * scalar;
}

Vec3 operator*(float scalar, const Vec3& vec) {
	return vec * scalar;
}

void	Scop::processMouseScroll(double yoffset)
{
	if (this->distanceFromCube >= 1.0f && this->distanceFromCube <= 45.0f)
		this->distanceFromCube -= static_cast<float>(yoffset);
	if (this->distanceFromCube < 1.0f)
		this->distanceFromCube = 1.0f;
	if (this->distanceFromCube > 45.0f)
		this->distanceFromCube = 45.0f;
}

void	Scop::cameraMovement()
{
	double xPos, yPos;
	glfwGetCursorPos(this->window, &xPos, &yPos);

	if (this->firstMouse)
	{
		this->lastX = xPos;
		this->lastY = yPos;
		this->firstMouse = false;
	}

	float xOffset = xPos - this->lastX;
	float yOffset = this->lastY - yPos;
	xOffset *= this->sensitivity;
	yOffset *= this->sensitivity;

	if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		xOffset = 0.0f;
		yOffset = 0.0f;
		this->lastX = xPos;
		this->lastY = yPos;
	}

	this->yaw += xOffset;
	this->pitch += yOffset;

	if (this->pitch > 89.0f)
		this->pitch = 89.0f;

	if (this->pitch < -89.0f)
		this->pitch = -89.0f;

	Vec3 front(0.0f, 0.0f, 0.0f);
	front.x = cos(toRadians(this->yaw)) * cos(toRadians(this->pitch));
	front.y = sin(toRadians(this->pitch));
	front.z = sin(toRadians(this->yaw)) * cos(toRadians(this->pitch));
	this->cameraFront = Vec3::normalize(front);

	this->cameraPos = this->cameraTarget - this->cameraFront * this->distanceFromCube;
	this->view = Mat4::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
}

void Scop::objectMovement()
{
	Mat3 rotationMatrix3 = Mat3(this->model);
	Mat3 invRotationMatrix = Mat3::transpose(rotationMatrix3);

	Vec3 translationDelta(0.0f, 0.0f, 0.0f);

	if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS)
		translationDelta += movementSpeed * invRotationMatrix * Vec3(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS)
		translationDelta -= movementSpeed * invRotationMatrix * Vec3(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS)
		translationDelta -= movementSpeed * invRotationMatrix * Vec3(1.0f, 0.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS)
		translationDelta += movementSpeed * invRotationMatrix * Vec3(1.0f, 0.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS)
		translationDelta += movementSpeed * Vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS)
		translationDelta -= movementSpeed * Vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translationDelta = -objectPosition;

	this->objectPosition += translationDelta;
	this->rotationAngle = this->rotationSpeed * this->deltaTime;

	Vec3 modelCenterOffset = this->calculateModelCenterOffset() - this->objectPosition;
	Mat4 translationToOrigin = Mat4::translate(-modelCenterOffset);
	Mat4 translationBack = Mat4::translate(modelCenterOffset);
	Mat4 rotationMatrix4 = Mat4::rotateY(this->rotationAngle);

	this->model = Mat4::translate(this->objectPosition) * translationBack * Mat4(rotationMatrix3) * rotationMatrix4 * translationToOrigin;
}

Vec3 Scop::calculateModelCenterOffset()
{
	Vec3 min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	Vec3 max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const Vec3& vertex : vertex_postitions)
	{
		min = Vec3::min(min, vertex);
		max = Vec3::max(max, vertex);
	}

	Vec3 center = (min + max) * 0.5f;

	return -center;
}
