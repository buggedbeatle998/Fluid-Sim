#include <vk_types.h>
#include <SDL_events.h>

class Camera {
public:
    bool movable{ false };
    float speed{ 0.1f };
    glm::vec3 velocity;
    glm::vec3 position;
    uint8_t keys_pressed{ 0 };
    SDL_bool unlock_mouse{ SDL_TRUE };
    // vertical rotation
    float pitch{ 0.f };
    // horizontal rotation
    float yaw{ 0.f };

    void setMovable(bool toMovable);
    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();
    glm::mat4 getYawRotationMatrix();

    void processSDLEvent(SDL_Event& e);

    void update();
};
