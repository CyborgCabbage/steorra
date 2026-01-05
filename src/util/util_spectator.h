#pragma once
#include <glm/mat4x4.hpp>

class Spectator {
public:
	Spectator();
	void Turn(glm::dvec2 rel);
	void Move(glm::dvec3 rel);
	glm::dmat4 GetViewMatrix() const;
	glm::dvec3 GetForwardVector() const;
	glm::dvec3 GetRightVector() const;
	glm::dvec3 position;
	double yaw, pitch;
};