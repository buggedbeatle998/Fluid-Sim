#include <camera.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::update()
{
    glm::mat4 cameraRotation = getYawRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}

void Camera::processSDLEvent(SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_w) {
                keys_pressed |= 1;
            }
            if (e.key.keysym.sym == SDLK_s) {
                keys_pressed |= 2;
            }
            if (e.key.keysym.sym == SDLK_a) {
                keys_pressed |= 4;
            }
            if (e.key.keysym.sym == SDLK_d) {
                keys_pressed |= 8;
            }
            if (e.key.keysym.sym == SDLK_LSHIFT) {
                keys_pressed |= 16;
            }
            if (e.key.keysym.sym == SDLK_SPACE) {
                keys_pressed |= 32;
            }
        }

        if (e.type == SDL_KEYUP) {
            if (e.key.keysym.sym == SDLK_w) {
                keys_pressed &= ~1;
            }
            if (e.key.keysym.sym == SDLK_s) {
                keys_pressed &= ~2;
            }
            if (e.key.keysym.sym == SDLK_a) {
                keys_pressed &= ~4;
            }
            if (e.key.keysym.sym == SDLK_d) {
                keys_pressed &= ~8;
            }
            if (e.key.keysym.sym == SDLK_LSHIFT) {
                keys_pressed &= ~16;
            }
            if (e.key.keysym.sym == SDLK_SPACE) {
                keys_pressed &= ~32;
            }
        }

        if (e.key.keysym.sym == SDLK_w || e.key.keysym.sym == SDLK_s) {
            velocity.z = ((keys_pressed & 2) >> 1) - ((keys_pressed & 1) >> 0);
        }
        if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_d) {
            velocity.x = ((keys_pressed & 8) >> 3) - ((keys_pressed & 4) >> 2);
        }
        if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_SPACE) {
            velocity.y = ((keys_pressed & 32) >> 5) - ((keys_pressed & 16) >> 4);
        }

        if (e.key.keysym.sym == SDLK_u) {
            unlock_mouse = e.type == SDL_KEYUP ? SDL_TRUE : SDL_FALSE;
            SDL_SetRelativeMouseMode(unlock_mouse);
        }
    }

    if (unlock_mouse == SDL_TRUE && e.type == SDL_MOUSEMOTION) {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}

glm::mat4 Camera::getViewMatrix()
{
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix()
{
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix

    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{ 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

glm::mat4 Camera::getYawRotationMatrix()
{
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{ 0.f, -1.f, 0.f });

    return glm::toMat4(yawRotation);
}