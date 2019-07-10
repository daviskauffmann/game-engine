#ifndef ENGINE_PROGRAM_H
#define ENGINE_PROGRAM_H

struct program
{
    GLuint program_id;
};

struct program ENGINE_API *program_create(const char *vertex_filename, const char *fragment_filename);
GLint ENGINE_API program_get_location(struct program *program, const char *name);
void ENGINE_API program_bind(struct program *program);
void ENGINE_API program_set_int(struct program *program, const char *name, int value);
void ENGINE_API program_set_unsigned_int(struct program *program, const char *name, unsigned int value);
void ENGINE_API program_set_float(struct program *program, const char *name, float value);
void ENGINE_API program_set_vec3(struct program *program, const char *name, vec3 vec);
void ENGINE_API program_set_vec4(struct program *program, const char *name, vec4 vec);
void ENGINE_API program_set_mat4(struct program *program, const char *name, mat4 mat);
void ENGINE_API program_unbind(void);
void ENGINE_API program_destroy(struct program *program);

#endif
