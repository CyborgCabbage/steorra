#include "util_spectator.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

Spectator::Spectator() : position(0.0), yaw(0.0), pitch(0.0) {
}

void Spectator::Turn(glm::dvec2 radians) {
	yaw = glm::wrapAngle(yaw + radians.x);
	pitch = glm::clamp(pitch + radians.y, -glm::half_pi<double>(), glm::half_pi<double>());
}

void Spectator::Move(glm::dvec3 rel) {
	position += GetForwardVector() * rel.x;
	position += GetRightVector() * rel.y;
	position.z += rel.z;
}

glm::dmat4 Spectator::GetViewMatrix() const {
	glm::dvec3 dir(1.0, 0.0, 0.0);
	dir = glm::rotateY(dir, pitch);
	dir = glm::rotateZ(dir, yaw);
	return glm::lookAt(position, position + dir, glm::dvec3(0.0, 0.0, 1.0));
}

glm::dvec3 Spectator::GetForwardVector() const {
	return glm::rotateZ(glm::dvec3(1.0, 0.0, 0.0), yaw);
}

glm::dvec3 Spectator::GetRightVector() const {
	return glm::rotateZ(glm::dvec3(0.0, 1.0, 0.0), yaw);
}
