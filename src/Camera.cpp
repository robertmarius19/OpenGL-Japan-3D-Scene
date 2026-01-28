#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace gps {

    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = glm::normalize(cameraUp);

        // Compute initial camera direction vectors
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    // return the view matrix
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    // Camera movement
    void Camera::move(MOVE_DIRECTION direction, float speed) {

        if (direction == MOVE_FORWARD) {
            cameraPosition += cameraFrontDirection * speed;
        }
        if (direction == MOVE_BACKWARD) {
            cameraPosition -= cameraFrontDirection * speed;
        }
        if (direction == MOVE_RIGHT) {
            cameraPosition += cameraRightDirection * speed;
        }
        if (direction == MOVE_LEFT) {
            cameraPosition -= cameraRightDirection * speed;
        }

        // Keep target updated
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    // Camera rotation using yaw and pitch angles
    void Camera::rotate(float pitch, float yaw) {

        // Convert spherical angles ? Cartesian direction vector
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(direction);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        cameraTarget = cameraPosition + cameraFrontDirection;
    }
    glm::vec3 Camera::getPosition() {
        return cameraPosition;
    }
    void Camera::setPosition(glm::vec3 position) {
        this->cameraPosition = position;
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }

}
