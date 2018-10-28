#include <engine/engine.h>

static SDL_Window *window;
static SDL_GLContext context;

int window_init(const char *title, int width, int height)
{
    // create window
    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        error(SDL_GetError());

        return 1;
    }

    // create OpenGL context
    context = SDL_GL_CreateContext(window);

    if (!context)
    {
        error(SDL_GetError());

        return 1;
    }

    // init GLEW
    {
        GLenum glewError = glewInit();

        if (glewError != GLEW_OK)
        {
            error(glewGetErrorString(glewError));

            return 1;
        }
    }

    info("GLEW %s", glewGetString(GLEW_VERSION));
    info("OpenGL %s", glGetString(GL_VERSION));
    info("Vendor %s", glGetString(GL_VENDOR));
    info("Renderer %s", glGetString(GL_RENDERER));
    info("GLSL %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return 0;
}

const char *window_get_title(void)
{
    return SDL_GetWindowTitle(window);
}

void window_set_title(const char *title)
{
    SDL_SetWindowTitle(window, title);
}

void window_get_size(int *width, int *height)
{
    SDL_GetWindowSize(window, width, height);
}

float window_get_aspect(void)
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    return (float)width / (float)height;
}

unsigned int window_get_flags(void)
{
    return SDL_GetWindowFlags(window);
}

void window_set_fullscreen(unsigned int flags)
{
    SDL_SetWindowFullscreen(window, flags);
}

void window_resize(int width, int height)
{
    SDL_SetWindowSize(window, width, height);

    info("Window resized to %dx%d", width, height);
}

void window_swap(void)
{
    SDL_GL_SwapWindow(window);
}

void window_quit(void)
{
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
}
