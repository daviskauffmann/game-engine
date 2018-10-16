#include <engine/error.h>
#include <engine/program.h>
#include <malloc.h>
#include <SDL/SDL.h>
#include <string.h>

static GLuint shader_create(GLenum type, const char *file);

struct program *program_create(const char *vertex_file, const char *fragment_file)
{
    struct program *program = malloc(sizeof(struct program));

    if (!program)
    {
        error_set("Couldn't allocate program");

        return NULL;
    }

    program->program = glCreateProgram();

    GLuint vertex_shader = 0;
    if (vertex_file)
    {
        vertex_shader = shader_create(GL_VERTEX_SHADER, vertex_file);

        if (!vertex_shader)
        {
            return NULL;
        }

        glAttachShader(program->program, vertex_shader);

        glDeleteShader(vertex_shader);
    }

    GLuint fragment_shader = 0;
    if (fragment_file)
    {
        fragment_shader = shader_create(GL_FRAGMENT_SHADER, fragment_file);

        if (!fragment_shader)
        {
            return NULL;
        }

        glAttachShader(program->program, fragment_shader);

        glDeleteShader(fragment_shader);
    }

    glLinkProgram(program->program);
    {
        GLint success;
        glGetProgramiv(program->program, GL_LINK_STATUS, &success);

        if (!success)
        {
            GLchar info_log[512];
            glGetProgramInfoLog(program->program, sizeof(info_log), NULL, info_log);

            error_set("Program linking failed\n%s", info_log);

            return NULL;
        }
    }

    if (vertex_shader)
    {
        glDetachShader(program->program, vertex_shader);
    }

    if (fragment_shader)
    {
        glDetachShader(program->program, fragment_shader);
    }

    glValidateProgram(program->program);
    {
        GLint success;
        glGetProgramiv(program->program, GL_VALIDATE_STATUS, &success);

        if (!success)
        {
            GLchar info_log[512];
            glGetProgramInfoLog(program->program, sizeof(info_log), NULL, info_log);

            error_set("Program validation failed\n%s", info_log);

            return NULL;
        }
    }

    return program;
}

GLint program_get_location(struct program *program, const char *name)
{
    return glGetUniformLocation(program->program, name);
}

void program_bind(struct program *program)
{
    glUseProgram(program->program);
}

void program_set_int(GLint location, int value)
{
    glUniform1i(location, value);
}

void program_set_float(GLint location, float value)
{
    glUniform1f(location, value);
}

void program_set_vec3(GLint location, vec3 vec)
{
    glUniform3f(location, vec[0], vec[1], vec[2]);
}

void program_set_mat4(GLint location, mat4 mat)
{
    glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat *)mat);
}

void program_unbind(void)
{
    glUseProgram(0);
}

void program_destroy(struct program *program)
{
    glDeleteProgram(program->program);

    free(program);
}

static GLuint shader_create(GLenum type, const char *file)
{
    GLuint shader = glCreateShader(type);

    SDL_RWops *io = SDL_RWFromFile(file, "rb");

    if (!io)
    {
        error_set("Couldn't open file %s", file);

        return 0;
    }

    SDL_RWseek(io, 0, RW_SEEK_END);
    size_t size = (size_t)SDL_RWtell(io);
    SDL_RWseek(io, 0, RW_SEEK_SET);

    char *source = malloc(size + 1);

    if (!source)
    {
        error_set("Couldn't allocate size %ld", size);

        return 0;
    }

    if (SDL_RWread(io, source, size, 1) <= 0)
    {
        error_set("Couldn't read file %s", file);

        return 0;
    }

    source[size] = '\0';

    SDL_RWclose(io);

    glShaderSource(shader, 1, &source, NULL);

    free(source);

    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLchar info_log[512];
        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);

        error_set("Shader compilation failed\n%s", info_log);

        return 0;
    }

    return shader;
}
