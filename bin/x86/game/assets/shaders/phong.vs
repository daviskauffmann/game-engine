#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform struct Camera
{
    mat4 projection;
    mat4 view;
    vec3 position;
} camera;

uniform struct Object
{
    mat4 model;
} object;

uniform struct Sun
{
    mat4 projection;
    mat4 view;
} sun;

out struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec4 light_space_position;
} vertex;

void main()
{
    gl_Position = camera.projection * camera.view * object.model * vec4(position, 1.0);
    vertex.position = vec3(object.model * vec4(position, 1.0));
    vertex.normal = mat3(transpose(inverse(object.model))) * normal;
    vertex.uv = vec2(uv.x, 1 - uv.y);
    vertex.light_space_position = sun.projection * sun.view * vec4(vertex.position, 1.0);
}
