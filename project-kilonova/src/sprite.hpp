#ifndef GAME_SPRITE_HPP
#define GAME_SPRITE_HPP

#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "texture.hpp"

namespace pk
{
struct sprite
{
    pk::texture *color_map;
    glm::vec3 color;
    glm::vec2 position;
    float rotation;
    glm::vec2 scale;

    sprite(
        pk::texture *color_map,
        glm::vec3 color,
        glm::vec2 position,
        float rotation,
        glm::vec2 scale);

    glm::mat4 calc_model() const;
};
} // namespace pk

#endif
