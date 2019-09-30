#include "terrain.hpp"

#include <glm/gtc/matrix_transform.hpp>

constexpr float size = 100;
constexpr int vertex_count = 128;

namespace pk
{
terrain::terrain(int grid_x, int grid_z, pk::material *material)
    : position(glm::vec3(grid_x * size, 0, grid_z * size)),
      material(material)
{
    std::vector<pk::vertex> vertices;
    std::vector<unsigned int> indices;
    for (int i = 0; i < vertex_count; i++)
    {
        for (int j = 0; j < vertex_count; j++)
        {
            glm::vec3 position(
                -(float)j / ((float)vertex_count - 1) * size,
                sinf((float)(rand() % RAND_MAX)) - 2.0f,
                -(float)i / ((float)vertex_count - 1) * size);
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            glm::vec2 uv(
                (float)j / ((float)vertex_count - 1),
                (float)i / ((float)vertex_count - 1));
            auto vertex = pk::vertex(position, normal, uv);
            vertices.push_back(vertex);
        }
    }
    for (unsigned int z = 0; z < vertex_count - 1; z++)
    {
        for (unsigned int x = 0; x < vertex_count - 1; x++)
        {
            unsigned int top_left = (z * vertex_count) + x;
            unsigned int top_right = top_left + 1;
            unsigned int bottom_left = ((z + 1) * vertex_count) + x;
            unsigned int bottom_right = bottom_left + 1;
            indices.push_back(top_left);
            indices.push_back(bottom_left);
            indices.push_back(top_right);
            indices.push_back(top_right);
            indices.push_back(bottom_left);
            indices.push_back(bottom_right);
        }
    }
    this->mesh = new pk::mesh(vertices, indices);
}

terrain::~terrain()
{
    delete this->mesh;
}

glm::mat4 terrain::calc_model() const
{
    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::translate(model, this->position);
    return model;
}
} // namespace pk
