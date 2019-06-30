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
    sampler2D diffuse;
    sampler2D specular;
	float shininess;
    sampler2D normal;
    sampler2D emission;
	float glow;
} material;

uniform struct Sun
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	mat4 projection;
	mat4 view;
} sun;

uniform struct Depthmap {
    sampler2D texture;
} depthmap;

out vec4 frag_color;

void main()
{
	vec3 position = vertex.position;
	vec3 diffuse = texture(material.diffuse, vertex.uv).rgb * material.color;
	vec3 normal = normalize(vertex.normal);
	vec3 specular = texture(material.specular, vertex.uv).rgb;
	float shininess = material.shininess;
	vec3 emission = texture(material.emission, vertex.uv).rgb;
	float glow = material.glow;
    vec3 view_direction = normalize(camera.position - position);

    // ambient
    vec3 final_ambient = sun.ambient * diffuse;

    // diffuse
    vec3 light_direction = normalize(-sun.direction);
    float diffuse_factor = max(dot(normal, light_direction), 0.0);
    vec3 final_diffuse = sun.diffuse * diffuse * diffuse_factor;

    // specular
    vec3 halfway_direction = normalize(light_direction + view_direction);
	float specular_angle = max(dot(normal, halfway_direction), 0.0);
    float specular_factor = pow(specular_angle, shininess);
    vec3 final_specular = sun.specular * specular * specular_factor;

    // shadow
    vec4 shadow_position = sun.projection * sun.view * vec4(position, 1.0);
    vec3 proj_coords = (shadow_position.xyz / shadow_position.w) * 0.5 + 0.5;
    float current_depth = proj_coords.z;
    float bias = max(0.05 * (1.0 - dot(normal, light_direction)), 0.005); 
    float shadow = 0.0;
    vec2 texel_size = 1.0 / textureSize(depthmap.texture, 0);
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float pcf_depth = texture(depthmap.texture, proj_coords.xy + vec2(x, y) * texel_size).r;
            shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    if (proj_coords.z > 1.0) shadow = 0.0;

    frag_color = vec4((final_ambient + (1.0 - shadow) * (final_diffuse + final_specular)), 1.0);
}