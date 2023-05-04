#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include <iostream>

Camera::Camera(vec3 position, float fov)
{
	SetPosition(position);
	SetFOV(fov);

	//default values
	Rotation.y = 0.f;
	Rotation.x = -90.f;
	cachedMouseLocation.x = 400;
	cachedMouseLocation.y = 300;
	bFirstMouse = true;
	Sensitivity = .1f;
}

void Camera::SetPosition(vec3 position)
{
	Position = position;
}

void Camera::MoveCameraInViewSpace(float yaw, float pitch)
{

	vec3 direction;
	direction.x = cos(radians(yaw)) * cos(radians(pitch));
	direction.y = sin(radians(pitch));
	direction.z = sin(radians(yaw)) * cos(radians(pitch));
	frontVector = normalize(direction);
	rightVector = normalize(cross(frontVector, vec3(0.0, 1.0, 0.0))); //recalculate right vector
}

void Camera::KeyboardMovement(EMovementDirection direction, float deltaTime, unsigned int uniformBuffer)
{
	float Speed = 2.f * deltaTime;
	vec3 cameraUp = vec3(0.0, 1.0, 0.0);
	switch (direction)
	{
	case FORWARD:
		OffsetPosition(vec3(GetForwardVector().x, 0.0, GetForwardVector().z) * Speed);
		break;
	case BACKWARD:
		OffsetPosition(-(vec3(GetForwardVector().x, 0.0, GetForwardVector().z) * Speed));
		break;
	case RIGHT:
		OffsetPosition(normalize(cross(GetForwardVector(), cameraUp)) * Speed);
		break;
	case LEFT:
		OffsetPosition(-(normalize(cross(GetForwardVector(), cameraUp)) * Speed));
		break;
	}

	//if using uniformBuffer, then update the memory accordingly
	if (uniformBuffer == 0)
	{
		//no uniform buffer passed through / empty uniform buffer passed through
		return;
	}

	//Update view matrix in uniform buffer
	mat4 view = GetViewMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

vec3 Camera::GetPosition()
{
	return Position;
}

void Camera::OffsetPosition(vec3 position)
{
	Position += position;
}

vec3 Camera::GetForwardVector()
{
	return frontVector;
}

vec3 Camera::GetRightVector()
{
	return rightVector;
}

void Camera::CalculateMouseAdjustment(double currentYaw, double currentPitch)
{
	if (bFirstMouse)
	{
		cachedMouseLocation.x = currentYaw;
		cachedMouseLocation.y = currentPitch;
		bFirstMouse = false;
	}

	//Get new location of the mouse
	float xOffset = currentYaw - cachedMouseLocation.x;
	float yOffset = cachedMouseLocation.y - currentPitch; //Reversed as this is how OpenGL handled the Y coordiante
	//Update cached location of the mouse
	cachedMouseLocation.x = currentYaw;
	cachedMouseLocation.y = currentPitch;

	//Multiply change mouse position by sensitivity
	xOffset *= Sensitivity;
	yOffset *= Sensitivity;
	//Pass though mouse change to global values
	Rotation.x += xOffset;
	Rotation.y += yOffset;

	//clamp pitch values 
	Rotation.y = clamp(Rotation.y, -89.0f, 89.0f);
	
	MoveCameraInViewSpace(Rotation.x, Rotation.y);
}

void Camera::SetFOV(float fov)
{
	FOV = fov;
}

void Camera::AdjustFOV(float FOVchange, float maxFOV)
{
	FOV -= FOVchange;
	//clamp fov value between a range
	FOV = clamp(FOV, 1.0f, maxFOV);
}

float Camera::GetFOV()
{
	return radians(FOV);
}

mat4 Camera::GetViewMatrix()
{
	return lookAt(Position, Position + frontVector, vec3(0.0, 1.0, 0.0));
}
