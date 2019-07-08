#version 460 core

in struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 uv;
} vertex;

uniform struct Camera
{
    mat4 projection;
    mat4 view;
    vec3 position;
} camera;

uniform struct Material
{
    vec3 color;
    sampler2D diffuse_map;
    sampler2D specular_map;
	float shininess;
    sampler2D normal_map;
    sampler2D emission_map;
	float glow;
} material;

uniform struct DirectionalLight
{
    vec3 direction;
    vec3 ambient_color;
    vec3 diffuse_color;
    vec3 specular_color;
} directional_light;

out vec4 frag_color;

void main()
{
    // ambient
	vec3 diffuse_color = texture(material.diffuse_map, vertex.uv).rgb * material.color;
    vec3 final_ambient_color = directional_light.ambient_color * diffuse_color;

    // diffuse
    vec3 light_direction = normalize(-directional_light.direction);
	vec3 normal = normalize(vertex.normal);
    float diffuse_factor = max(dot(normal, light_direction), 0.0);
    vec3 final_diffuse_color = directional_light.diffuse_color * diffuse_color * diffuse_factor;

    // specular
    vec3 view_direction = normalize(camera.position - vertex.position);
    vec3 halfway_direction = normalize(light_direction + view_direction);
	float specular_angle = max(dot(normal, halfway_direction), 0.0);
	float shininess = material.shininess;
    float specular_factor = pow(specular_angle, shininess);
	vec3 specular_color = texture(material.specular_map, vertex.uv).rgb;
    vec3 final_specular_color = directional_light.specular_color * specular_color * specular_factor;

    frag_color = vec4(final_ambient_color + final_diffuse_color + final_specular_color, 1.0);
}
