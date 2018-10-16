#include <engine/error.h>
#include <engine/window.h>
#include <GL/glew.h>
#include <stdio.h>

SDL_Window *window;
SDL_GLContext context;

int window_init(const char *title, int width, int height)
{
    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        error_set(SDL_GetError());

        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    context = SDL_GL_CreateContext(window);

    if (!context)
    {
        error_set(SDL_GetError());

        return 1;
    }

    {
        GLenum error = glewInit();

        if (error != GLEW_OK)
        {
            error_set(glewGetErrorString(error));

            return 1;
        }
    }

    printf("GLEW %s\n", glewGetString(GLEW_VERSION));
    printf("OpenGL %s\n", glGetString(GL_VERSION));

    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_CLAMP);
    glEnable(GL_TEXTURE_2D);

    return 0;
}

const unsigned char *window_keyboard(int *num_keys)
{
    return SDL_GetKeyboardState(num_keys);
}

unsigned int window_mouse(int *mouse_x, int *mouse_y)
{
    return SDL_GetMouseState(mouse_x, mouse_y);
}

int window_event(SDL_Event *event)
{
    return SDL_PollEvent(event);
}

const char *window_get_title(void)
{
    return SDL_GetWindowTitle(window);
}

void window_toggle_mouse()
{
    SDL_SetRelativeMouseMode(!SDL_GetRelativeMouseMode());
}

void window_set_title(const char *title)
{
    SDL_SetWindowTitle(window, title);
}

void window_toggle_fullscreen(void)
{
    unsigned int flags = SDL_GetWindowFlags(window);

    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
    {
        SDL_SetWindowFullscreen(window, 0);
    }
    else
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void window_resize(int width, int height)
{
    SDL_SetWindowSize(window, width, height);
    glViewport(0, 0, width, height);

    printf("Window resized to %dx%d\n", width, height);
}

float window_aspect_ratio(void)
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    return (float)width / (float)height;
}

void window_clear(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void window_render(void)
{
    SDL_GL_SwapWindow(window);
}

void window_quit(void)
{
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
}
