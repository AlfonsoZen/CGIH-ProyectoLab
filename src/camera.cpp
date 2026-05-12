#include "camera.h"
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 up, GLfloat yaw, GLfloat pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM)
{
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
{
    GLfloat   velocity  = movementSpeed * deltaTime;
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

    if (direction == FORWARD)  position += flatFront * velocity;
    if (direction == BACKWARD) position -= flatFront * velocity;
    if (direction == LEFT)     position -= right * velocity;
    if (direction == RIGHT)    position += right * velocity;
}

void Camera::ProcessMouseMovement(GLfloat xOffset, GLfloat yOffset)
{
    yaw   += xOffset * mouseSensitivity;
    pitch += yOffset * mouseSensitivity;

    if (pitch >  PITCH_LIMIT) pitch =  PITCH_LIMIT;
    if (pitch < -PITCH_LIMIT) pitch = -PITCH_LIMIT;

    updateCameraVectors();
}

GLfloat   Camera::GetZoom()     { return zoom; }
glm::vec3 Camera::GetPosition() { return position; }
glm::vec3 Camera::GetFront()    { return front; }

void Camera::updateCameraVectors()
{
    glm::vec3 f;
    f.x   = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y   = sin(glm::radians(pitch));
    f.z   = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}
