#include <engine/engine.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#define WINDOW_TITLE "Breakout"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define RENDER_SCALE 1.0f

#define SHADOW_WIDTH 4096
#define SHADOW_HEIGHT 4096

#define FPS_CAP 120
#define FRAME_DELAY (1000 / FPS_CAP)

enum game_state
{
    GAME_STATE_ACTIVE,
    GAME_STATE_MENU,
    GAME_STATE_WIN
};

static enum game_state game_state;

int main(int argc, char *argv[])
{
    // init SDL
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    // create window
    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);

    // init engine
    renderer_init(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        RENDER_SCALE,
        SHADOW_WIDTH,
        SHADOW_HEIGHT);

    // create textures
    SDL_Surface *awesomeface_surface = IMG_Load("assets/images/awesomeface.png");

    struct texture *awesomeface_texture = texture_create(
        awesomeface_surface->w,
        awesomeface_surface->h,
        awesomeface_surface->format->BytesPerPixel,
        awesomeface_surface->pixels);

    SDL_FreeSurface(awesomeface_surface);

    // create sprites
    vec3 awesomeface_sprite_color = { 1.0f, 1.0f, 1.0f };
    vec2 awesomeface_sprite_position = { 0.0f, 0.0f };
    float awesomeface_sprite_rotation = 0.0f;
    vec2 awesomeface_sprite_scale = { 1.0f, 1.0f };

    struct sprite *awesomeface_sprite = sprite_create(
        awesomeface_sprite_color,
        awesomeface_texture,
        awesomeface_sprite_position,
        awesomeface_sprite_rotation,
        awesomeface_sprite_scale);

    // create camera
    vec3 camera_position = { 0.0f, 0.0f, 3.0f };
    vec3 camera_front = { 0.0f, 0.0f, -1.0f };
    vec3 camera_up = { 0.0f, 1.0f, 0.0f };

    struct camera *camera = camera_create(
        camera_position,
        camera_front,
        camera_up,
        0.0f,
        -90.0f,
        0.0f,
        45.0f);

    // game settings
    unsigned int current_time = 0;
    float fps_update_timer = 0.0f;

    renderer_set_mode(RENDER_MODE_FORWARD);

    // main loop
    bool quit = false;
    while (!quit)
    {
        // timer for fps cap
        unsigned int frame_start = SDL_GetTicks();

        // calculate time passed since last frame
        unsigned int previous_time = current_time;
        current_time = frame_start;

        // calculate delta time and fps
        float delta_time = (current_time - previous_time) / 1000.0f;
        unsigned int fps = (unsigned int)(1 / delta_time);

        // update window title
        fps_update_timer += delta_time;

        if (fps_update_timer > 0.25f)
        {
            fps_update_timer = 0.0f;

            char title[256];
            sprintf_s(title, sizeof(title), "%s - FPS: %d", WINDOW_TITLE, fps);
            SDL_SetWindowTitle(window, title);
        }

        // get keyboard input
        int num_keys;
        const unsigned char *keys = SDL_GetKeyboardState(&num_keys);

        // get mouse input
        int mouse_x, mouse_y;
        unsigned int mouse = SDL_GetMouseState(&mouse_x, &mouse_y);

        // handle events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_F4:
                {
                    if (keys[SDL_SCANCODE_LALT])
                    {
                        quit = true;
                    }
                }
                break;
                case SDLK_RETURN:
                {
                    if (keys[SDL_SCANCODE_LALT])
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
                }
                break;
                }
                break;
            case SDL_QUIT:
            {
                quit = true;
            }
            break;
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    int width = event.window.data1;
                    int height = event.window.data2;

                    SDL_SetWindowSize(window, width, height);

                    printf("Window resized to %dx%d\n", width, height);
                }
                break;
                }
            }
            break;
            }
            }
        }

        // update sprite
        awesomeface_sprite->rotation += delta_time;

        // setup renderer
        renderer_add_sprite(awesomeface_sprite);

        // render everything
        renderer_draw(camera, false, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);

        // display the window
        SDL_GL_SwapWindow(window);
    }

    // free resources
    sprite_destroy(awesomeface_sprite);

    // close engine
    audio_quit();
    renderer_quit();

    // close window
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    // close SDL
    IMG_Quit();
    SDL_Quit();

    return 0;
}
