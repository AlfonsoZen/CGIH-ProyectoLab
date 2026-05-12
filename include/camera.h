#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

constexpr GLfloat YAW         = -90.0f;
constexpr GLfloat PITCH        =   0.0f;
constexpr GLfloat SPEED        =   6.0f;
constexpr GLfloat SENSITIVITY  =   0.08f;
constexpr GLfloat ZOOM         =  45.0f;
constexpr GLfloat PITCH_LIMIT  =  89.0f;

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    GLfloat yaw;
    GLfloat pitch;
    GLfloat movementSpeed;
    GLfloat mouseSensitivity;
    GLfloat zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f),
           glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f),
           GLfloat   yaw      = YAW,
           GLfloat   pitch    = PITCH);

    glm::mat4 GetViewMatrix();
    void      ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime);
    void      ProcessMouseMovement(GLfloat xOffset, GLfloat yOffset);
    GLfloat   GetZoom();
    glm::vec3 GetPosition();
    glm::vec3 GetFront();

private:
    void updateCameraVectors();
};
