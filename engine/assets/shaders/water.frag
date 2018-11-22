#version 460 core

in struct Vertex
{
    vec3 position;
    vec3 normal;
} vertex;

uniform struct Camera
{
    mat4 projection;
    mat4 view;
    vec3 position;
} camera;

uniform struct Skybox
{
	samplerCube texture;
} skybox;

out vec4 frag_color;

void main()
{    
    vec3 view = normalize(vertex.position - camera.position);
	
	vec3 reflection = reflect(view, normalize(vertex.normal));
	
	float ratio = 1.00 / 1.33;
	vec3 refraction = refract(view, normalize(vertex.normal), ratio);

    frag_color = vec4(texture(skybox.texture, reflection).rgb, 1.0);
}
