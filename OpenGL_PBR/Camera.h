#pragma once

#include <string>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

enum EMovementDirection
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

class Camera
{
private:

	vec3 Position;

	vec3 frontVector;

	vec3 rightVector;

	float FOV;

	vec3 Rotation;
	vec2 cachedMouseLocation;
	bool bFirstMouse;

	float Sensitivity;

public:
	Camera(vec3 position, float fov);

	//Override Camera Position
	void SetPosition(vec3 position);

	void CalculateMouseAdjustment(double currentYaw, double currentPitch);

	void MoveCameraInViewSpace(float yaw, float pitch);

	void KeyboardMovement(EMovementDirection direction, float deltaTime, unsigned int uniformBuffer = 0);

	//Add to the Current Position
	void OffsetPosition(vec3 position);

	void SetFOV(float fov);

	void AdjustFOV(float FOVChange, float maxFOV);

	vec3 GetPosition();
	vec3 GetForwardVector();
	vec3 GetRightVector();
	float GetFOV();
	mat4 GetViewMatrix();
};

