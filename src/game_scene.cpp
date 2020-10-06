#include "game_scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "pause_scene.hpp"

// TODO: read from file
pk::game_scene::game_scene()
{
    camera = new pk::camera(
        glm::vec3(0.0f, 0.0f, 3.0f),
        0.0f,
        -90.0f,
        0.0f,
        45.0f);

    skybox = new pk::skybox("assets/images/GCanyon_C_YumaPoint_8k.jpg");

    backpack_model = new pk::model("assets/models/backpack/backpack.obj");
    backpack = new pk::object(
        backpack_model,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f));

    const float sun_intensity = 10.0f;
    sun = new pk::directional_light(
        glm::vec3(0.352286f, -0.547564f, -0.758992f),
        glm::vec3(1.0f, 1.0f, 1.0f) * sun_intensity,
        4096);

    const float light_intensity = 10.0f;
    red_light = new pk::point_light(
        glm::vec3(2.0f, 0.0f, 2.0f),
        glm::vec3(1.0f, 0.0f, 0.0f) * light_intensity,
        512);
    yellow_light = new pk::point_light(
        glm::vec3(-2.0f, 0.0f, -2.0f),
        glm::vec3(1.0f, 1.0f, 0.0f) * light_intensity,
        512);
    green_light = new pk::point_light(
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(0.0f, 1.0f, 0.0f) * light_intensity,
        512);
    blue_light = new pk::point_light(
        glm::vec3(-2.0f, 0.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 1.0f) * light_intensity,
        512);

    const float torch_intensity = 20.0f;
    torch = new pk::spot_light(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f) * torch_intensity,
        cosf(glm::radians(12.5f)),
        cosf(glm::radians(15.0f)),
        1024);

    water = new pk::water(
        glm::vec3(0.0f, -2.0f, 0.0f),
        glm::vec2(100.0f, 100.0f));

    terrain = new pk::terrain(0, 0);

    ambient_source = new pk::source(glm::vec3(0.0f, 0.0f, 0.0f));
    bounce_source = new pk::source(glm::vec3(0.0f, 0.0f, 0.0f));
    shoot_source = new pk::source(glm::vec3(0.0f, 0.0f, 0.0f));

    ambient_sound = new pk::sound("assets/audio/ambient.wav");
    bounce_sound = new pk::sound("assets/audio/bounce.wav");
    shoot_sound = new pk::sound("assets/audio/shoot.wav");

    edit_mode = false;
    lock_cursor = true;
    torch_on = true;
    torch_follow = true;

    ambient_source->set_loop(true);
    ambient_source->set_gain(0.25f);
    ambient_source->play(ambient_sound);
}

pk::game_scene::~game_scene()
{
    delete camera;

    delete skybox;

    delete backpack_model;
    delete backpack;

    delete sun;

    delete red_light;
    delete yellow_light;
    delete green_light;
    delete blue_light;

    delete torch;

    delete water;

    delete terrain;

    delete ambient_source;
    delete bounce_source;
    delete shoot_source;

    delete ambient_sound;
    delete bounce_sound;
    delete shoot_sound;
}

pk::scene *pk::game_scene::handle_event(SDL_Event event)
{
    ImGuiIO &io = ImGui::GetIO();

    switch (event.type)
    {
    case SDL_KEYDOWN:
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_TAB:
        {
            if (!io.WantCaptureKeyboard)
            {
                lock_cursor = !lock_cursor;
            }
        }
        break;
        case SDLK_ESCAPE:
        {
            if (!io.WantCaptureKeyboard)
            {
                SDL_SetRelativeMouseMode(SDL_FALSE);

                return new pk::pause_scene(this);
            }
        }
        break;
        case SDLK_e:
        {
            if (!io.WantCaptureKeyboard)
            {
                edit_mode = !edit_mode;
            }
        }
        break;
        case SDLK_f:
        {
            if (!io.WantCaptureKeyboard)
            {
                torch_on = !torch_on;
            }
        }
        break;
        case SDLK_g:
        {
            if (!io.WantCaptureKeyboard)
            {
                torch_follow = !torch_follow;
            }
        }
        break;
        }
    }
    break;
    case SDL_MOUSEMOTION:
    {
        if (SDL_GetRelativeMouseMode() && !io.WantCaptureMouse)
        {
            camera->pitch -= event.motion.yrel * 0.1f;
            camera->yaw += event.motion.xrel * 0.1f;
            if (camera->pitch > 89.0f)
            {
                camera->pitch = 89.0f;
            }
            if (camera->pitch < -89.0f)
            {
                camera->pitch = -89.0f;
            }
        }
    }
    break;
    case SDL_MOUSEWHEEL:
    {
        if (!io.WantCaptureMouse)
        {
            camera->fov -= event.wheel.y;
            if (camera->fov <= 1.0f)
            {
                camera->fov = 1.0f;
            }
            if (camera->fov >= 120.0f)
            {
                camera->fov = 120.0f;
            }
        }
    }
    break;
    }

    return this;
}

pk::scene *pk::game_scene::update(pk::audio *audio, float delta_time)
{
    ImGuiIO &io = ImGui::GetIO();

    SDL_SetRelativeMouseMode((SDL_bool)lock_cursor);

    int num_keys;
    const unsigned char *keys = SDL_GetKeyboardState(&num_keys);
    int mouse_x, mouse_y;
    unsigned int mouse = SDL_GetMouseState(&mouse_x, &mouse_y);

    glm::vec3 camera_front = camera->calc_front();
    glm::vec3 camera_right = camera->calc_right();

    static glm::vec3 velocity(0.0f, 0.0f, 0.0f);
    glm::vec3 acceleration(0.0f, 0.0f, 0.0f);
    float speed = 50.0f;
    if (!io.WantCaptureKeyboard)
    {
        if (keys[SDL_SCANCODE_W])
        {
            acceleration += camera_front;
        }
        if (keys[SDL_SCANCODE_A])
        {
            acceleration -= camera_right;
        }
        if (keys[SDL_SCANCODE_S])
        {
            acceleration -= camera_front;
        }
        if (keys[SDL_SCANCODE_D])
        {
            acceleration += camera_right;
        }
        if (keys[SDL_SCANCODE_SPACE])
        {
            acceleration.y = 1.0f;
        }
        if (keys[SDL_SCANCODE_LCTRL])
        {
            acceleration.y = -1.0f;
        }
        if (keys[SDL_SCANCODE_LSHIFT])
        {
            speed *= 2.0f;
        }
    }
    float acceleration_length = glm::length(acceleration);
    if (acceleration_length > 1.0f)
    {
        acceleration /= acceleration_length;
    }
    acceleration *= speed;
    acceleration -= velocity * 10.0f;
    camera->position = 0.5f * acceleration * powf(delta_time, 2.0f) + velocity * delta_time + camera->position;
    velocity = acceleration * delta_time + velocity;
    // float velocity_length = glm::length(velocity);
    // camera->roll = 10.0f * glm::dot(main_camera_right, velocity) / velocity_length;

    static float angle = 0.0f;
    constexpr float pi = 3.14159f;
    float distance = 6.0f;
    angle += 0.5f * delta_time;
    if (angle > 2 * pi)
    {
        angle = 0;
    }
    red_light->position.x = distance * sinf(angle);
    red_light->position.z = distance * cosf(angle);
    yellow_light->position.x = distance * sinf(angle + pi / 2);
    yellow_light->position.z = distance * cosf(angle + pi / 2);
    green_light->position.x = distance * sinf(angle + pi);
    green_light->position.z = distance * cosf(angle + pi);
    blue_light->position.x = distance * sinf(angle + 3 * pi / 2);
    blue_light->position.z = distance * cosf(angle + 3 * pi / 2);

    if (torch_follow)
    {
        torch->position = camera->position;
        torch->direction = glm::mix(torch->direction, camera_front, 30.0f * delta_time);
    }

    audio->set_listener(camera->position, camera_front, glm::vec3(0.0f, 1.0f, 0.0f));
    ambient_source->set_position(camera->position);
    shoot_source->set_position(camera->position);

    if (!io.WantCaptureMouse)
    {
        if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            if (!shoot_source->is_playing())
            {
                shoot_source->play(shoot_sound);
            }
        }

        if (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT))
        {
            if (!bounce_source->is_playing())
            {
                bounce_source->play(bounce_sound);
            }
        }
    }

    return this;
}

void pk::game_scene::render(pk::renderer *renderer) const
{
    renderer->camera = camera;
    renderer->skybox = skybox;
    renderer->objects.push_back(backpack);
    renderer->directional_lights.push_back(sun);
    renderer->point_lights.push_back(red_light);
    renderer->point_lights.push_back(yellow_light);
    renderer->point_lights.push_back(green_light);
    renderer->point_lights.push_back(blue_light);
    if (torch_on)
    {
        renderer->spot_lights.push_back(torch);
    }
    renderer->waters.push_back(water);
}

void pk::game_scene::gui() const
{
    if (edit_mode)
    {
        // TODO: edit mode UI
    }
}

void pk::game_scene::print_commands()
{
}

void pk::game_scene::handle_command(const char *command)
{
}
