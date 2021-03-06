#include "renderer.hpp"

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// TODO: framebuffer helper class
// should store info about width/height
// when binding the framebuffer, automatically set viewport to those values
// and when unbinding, reset the viewport to some default value (probably the display width/height)

// TODO: control shadowmap resolutions from a centralized location
// currently they are stored on each instance of a light

// TODO: render_scale other than 1 causes water reflection/refraction to break

// TODO: print more specific errors when framebuffers fail

liminal::renderer::renderer(
    GLsizei display_width, GLsizei display_height, float render_scale,
    GLsizei reflection_width, GLsizei reflection_height,
    GLsizei refraction_width, GLsizei refraction_height)
{
    wireframe = false;
    greyscale = false;
    camera = nullptr;
    skybox = nullptr;

    // setup fbos
    hdr_fbo_id = 0;
    hdr_texture_ids[0] = 0;
    hdr_texture_ids[1] = 0;
    hdr_rbo_id = 0;
    geometry_fbo_id = 0;
    geometry_position_texture_id = 0;
    geometry_normal_texture_id = 0;
    geometry_albedo_texture_id = 0;
    geometry_material_texture_id = 0;
    geometry_rbo_id = 0;
    set_screen_size(display_width, display_height, render_scale);

    water_reflection_fbo_id = 0;
    water_reflection_color_texture_id = 0;
    water_reflection_rbo_id = 0;
    set_reflection_size(reflection_width, reflection_height);

    water_refraction_fbo_id = 0;
    water_refraction_color_texture_id = 0;
    water_refraction_depth_texture_id = 0;
    set_refraction_size(refraction_width, refraction_height);

    // init OpenGL state
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glCullFace(GL_BACK);
    // glEnable(GL_MULTISAMPLE);
    // glEnable(GL_FRAMEBUFFER_SRGB);

    // create water mesh
    glGenVertexArrays(1, &water_vao_id);
    glBindVertexArray(water_vao_id);
    {
        std::vector<GLfloat> water_vertices =
            {-1.0f, -1.0f,
             -1.0f, +1.0f,
             +1.0f, -1.0f,

             +1.0f, -1.0f,
             -1.0f, +1.0f,
             +1.0f, +1.0f};
        water_vertices_size = (GLsizei)(water_vertices.size() * sizeof(GLfloat));

        glGenBuffers(1, &water_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, water_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, water_vertices_size, water_vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)(0 * sizeof(GLfloat)));

        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(0);

    // create skybox mesh
    glGenVertexArrays(1, &skybox_vao_id);
    glBindVertexArray(skybox_vao_id);
    {
        std::vector<GLfloat> skybox_vertices =
            {-1.0f, +1.0f, -1.0f,
             -1.0f, -1.0f, -1.0f,
             +1.0f, -1.0f, -1.0f,
             +1.0f, -1.0f, -1.0f,
             +1.0f, +1.0f, -1.0f,
             -1.0f, +1.0f, -1.0f,

             -1.0f, -1.0f, +1.0f,
             -1.0f, -1.0f, -1.0f,
             -1.0f, +1.0f, -1.0f,
             -1.0f, +1.0f, -1.0f,
             -1.0f, +1.0f, +1.0f,
             -1.0f, -1.0f, +1.0f,

             +1.0f, -1.0f, -1.0f,
             +1.0f, -1.0f, +1.0f,
             +1.0f, +1.0f, +1.0f,
             +1.0f, +1.0f, +1.0f,
             +1.0f, +1.0f, -1.0f,
             +1.0f, -1.0f, -1.0f,

             -1.0f, -1.0f, +1.0f,
             -1.0f, +1.0f, +1.0f,
             +1.0f, +1.0f, +1.0f,
             +1.0f, +1.0f, +1.0f,
             +1.0f, -1.0f, +1.0f,
             -1.0f, -1.0f, +1.0f,

             -1.0f, +1.0f, -1.0f,
             +1.0f, +1.0f, -1.0f,
             +1.0f, +1.0f, +1.0f,
             +1.0f, +1.0f, +1.0f,
             -1.0f, +1.0f, +1.0f,
             -1.0f, +1.0f, -1.0f,

             -1.0f, -1.0f, -1.0f,
             -1.0f, -1.0f, +1.0f,
             +1.0f, -1.0f, -1.0f,
             +1.0f, -1.0f, -1.0f,
             -1.0f, -1.0f, +1.0f,
             +1.0f, -1.0f, +1.0f};
        skybox_vertices_size = (GLsizei)(skybox_vertices.size() * sizeof(GLfloat));

        glGenBuffers(1, &skybox_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, skybox_vertices_size, skybox_vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)(0 * sizeof(GLfloat)));

        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(0);

    // create sprite mesh
    glGenVertexArrays(1, &sprite_vao_id);
    glBindVertexArray(sprite_vao_id);
    {
        std::vector<GLfloat> sprite_vertices =
            {+0.0f, +1.0f, +0.0f, +1.0f,
             +1.0f, +0.0f, +1.0f, +0.0f,
             +0.0f, +0.0f, +0.0f, +0.0f,

             +0.0f, +1.0f, +0.0f, +1.0f,
             +1.0f, +1.0f, +1.0f, +1.0f,
             +1.0f, +0.0f, +1.0f, +0.0f};
        sprite_vertices_size = (GLsizei)(sprite_vertices.size() * sizeof(GLfloat));

        glGenBuffers(1, &sprite_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, sprite_vertices_size, sprite_vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(0 * sizeof(GLfloat)));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }
    glBindVertexArray(0);

    // create screen mesh
    glGenVertexArrays(1, &screen_vao_id);
    glBindVertexArray(screen_vao_id);
    {
        std::vector<GLfloat> screen_vertices =
            {-1.0f, -1.0f, +0.0f, +0.0f,
             -1.0f, +1.0f, +0.0f, +1.0f,
             +1.0f, -1.0f, +1.0f, +0.0f,
             +1.0f, -1.0f, +1.0f, +0.0f,
             -1.0f, +1.0f, +0.0f, +1.0f,
             +1.0f, +1.0f, +1.0f, +1.0f};
        screen_vertices_size = (GLsizei)(screen_vertices.size() * sizeof(GLfloat));

        glGenBuffers(1, &screen_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, screen_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, screen_vertices_size, screen_vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(0 * sizeof(GLfloat)));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }
    glBindVertexArray(0);

    // create brdf texture
    {
        const GLsizei brdf_size = 512;

        GLuint capture_fbo_id;
        GLuint capture_rbo_id;

        glGenFramebuffers(1, &capture_fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_id);
        {
            {
                glGenTextures(1, &brdf_texture_id);
                glBindTexture(GL_TEXTURE_2D, brdf_texture_id);
                {
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RG16F,
                        brdf_size,
                        brdf_size,
                        0,
                        GL_RG,
                        GL_FLOAT,
                        0);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }
                glBindTexture(GL_TEXTURE_2D, 0);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D,
                    brdf_texture_id,
                    0);
            }

            {
                glGenRenderbuffers(1, &capture_rbo_id);
                glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_id);
                {
                    glRenderbufferStorage(
                        GL_RENDERBUFFER,
                        GL_DEPTH_COMPONENT24,
                        brdf_size,
                        brdf_size);
                }
                glBindRenderbuffer(GL_RENDERBUFFER, 0);

                glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_RENDERBUFFER,
                    capture_rbo_id);
            }

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "Error: Failed to create brdf capture framebuffer" << std::endl;
                return;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        liminal::program *brdf_program = new liminal::program(
            "assets/shaders/brdf.vs",
            "assets/shaders/brdf.fs");

        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_id);
        {
            glViewport(0, 0, brdf_size, brdf_size);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            brdf_program->bind();
            {
                glBindVertexArray(screen_vao_id);
                glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
                glBindVertexArray(0);
            }
            brdf_program->unbind();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        delete brdf_program;

        glDeleteFramebuffers(1, &capture_fbo_id);
        glDeleteRenderbuffers(1, &capture_rbo_id);
    }

    // create shader programs
    depth_mesh_program = new liminal::program(
        "assets/shaders/depth_mesh.vs",
        "assets/shaders/depth.fs");
    depth_skinned_mesh_program = new liminal::program(
        "assets/shaders/depth_skinned_mesh.vs",
        "assets/shaders/depth.fs");
    depth_cube_mesh_program = new liminal::program(
        "assets/shaders/depth_cube_mesh.vs",
        "assets/shaders/depth_cube.gs",
        "assets/shaders/depth_cube.fs");
    depth_cube_skinned_mesh_program = new liminal::program(
        "assets/shaders/depth_cube_skinned_mesh.vs",
        "assets/shaders/depth_cube.gs",
        "assets/shaders/depth_cube.fs");
    color_program = new liminal::program(
        "assets/shaders/color.vs",
        "assets/shaders/color.fs");
    geometry_mesh_program = new liminal::program(
        "assets/shaders/geometry_mesh.vs",
        "assets/shaders/geometry_mesh.fs");
    geometry_skinned_mesh_program = new liminal::program(
        "assets/shaders/geometry_skinned_mesh.vs",
        "assets/shaders/geometry_mesh.fs");
    geometry_terrain_program = new liminal::program(
        "assets/shaders/geometry_mesh.vs",
        "assets/shaders/geometry_terrain.fs");
    deferred_ambient_program = new liminal::program(
        "assets/shaders/deferred.vs",
        "assets/shaders/deferred_ambient.fs");
    deferred_directional_program = new liminal::program(
        "assets/shaders/deferred.vs",
        "assets/shaders/deferred_directional.fs");
    deferred_point_program = new liminal::program(
        "assets/shaders/deferred.vs",
        "assets/shaders/deferred_point.fs");
    deferred_spot_program = new liminal::program(
        "assets/shaders/deferred.vs",
        "assets/shaders/deferred_spot.fs");
    skybox_program = new liminal::program(
        "assets/shaders/skybox.vs",
        "assets/shaders/skybox.fs");
    water_program = new liminal::program(
        "assets/shaders/water.vs",
        "assets/shaders/water.fs");
    sprite_program = new liminal::program(
        "assets/shaders/sprite.vs",
        "assets/shaders/sprite.fs");
    gaussian_program = new liminal::program(
        "assets/shaders/gaussian.vs",
        "assets/shaders/gaussian.fs");
    screen_program = new liminal::program(
        "assets/shaders/screen.vs",
        "assets/shaders/screen.fs");

    setup_samplers();

    // create water textures
    water_dudv_texture = new liminal::texture("assets/images/water_dudv.png");
    water_normal_texture = new liminal::texture("assets/images/water_normal.png");

    // DEBUG: create sphere mesh
    // http://www.songho.ca/opengl/gl_sphere.html
    {
        std::vector<liminal::vertex> vertices;
        int radius = 1;
        int stack_count = 36;
        int sector_count = 18;
        float length_inv = 1.0f / radius;
        float sector_step = 2 * 3.14f / sector_count;
        float stack_step = 3.14f / stack_count;
        for (int i = 0; i <= stack_count; i++)
        {
            float stack_angle = 3.14f / 2 - i * stack_step;
            float xy = radius * cosf(stack_angle);
            float z = radius * sinf(stack_angle);
            for (int j = 0; j <= sector_count; j++)
            {
                float sector_angle = j * sector_step;
                liminal::vertex vertex;
                vertex.position.x = xy * cosf(sector_angle);
                vertex.position.y = xy * sinf(sector_angle);
                vertex.position.z = z;
                vertex.normal.x = vertex.position.x * length_inv;
                vertex.normal.y = vertex.position.y * length_inv;
                vertex.normal.z = vertex.position.z * length_inv;
                vertex.uv.s = (float)j / sector_count;
                vertex.uv.t = (float)i / stack_count;
                vertices.push_back(vertex);
            }
        }

        std::vector<unsigned int> indices;
        for (int i = 0; i < stack_count; i++)
        {
            int k1 = i * (sector_count + 1);
            int k2 = k1 + sector_count + 1;
            for (int j = 0; j < sector_count; j++, k1++, k2++)
            {
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }
                if (i != stack_count - 1)
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        std::vector<std::vector<liminal::texture *>> textures;

        DEBUG_sphere_mesh = new liminal::mesh(vertices, indices, textures);
    }
}

liminal::renderer::~renderer()
{
    glDeleteFramebuffers(1, &geometry_fbo_id);
    glDeleteTextures(1, &geometry_position_texture_id);
    glDeleteTextures(1, &geometry_normal_texture_id);
    glDeleteTextures(1, &geometry_albedo_texture_id);
    glDeleteTextures(1, &geometry_material_texture_id);
    glDeleteRenderbuffers(1, &geometry_rbo_id);

    glDeleteFramebuffers(1, &hdr_fbo_id);
    glDeleteTextures(2, hdr_texture_ids);
    glDeleteRenderbuffers(1, &hdr_rbo_id);

    glDeleteFramebuffers(1, &water_reflection_fbo_id);
    glDeleteRenderbuffers(1, &water_reflection_rbo_id);
    glDeleteTextures(1, &water_reflection_color_texture_id);

    glDeleteFramebuffers(1, &water_refraction_fbo_id);
    glDeleteTextures(1, &water_refraction_color_texture_id);
    glDeleteTextures(1, &water_refraction_depth_texture_id);

    glDeleteFramebuffers(2, bloom_fbo_ids);
    glDeleteTextures(2, bloom_texture_ids);

    glDeleteVertexArrays(1, &water_vao_id);
    glDeleteBuffers(1, &water_vbo_id);

    glDeleteVertexArrays(1, &skybox_vao_id);
    glDeleteBuffers(1, &skybox_vbo_id);

    glDeleteVertexArrays(1, &sprite_vao_id);
    glDeleteBuffers(1, &sprite_vbo_id);

    glDeleteVertexArrays(1, &screen_vao_id);
    glDeleteBuffers(1, &screen_vbo_id);

    glDeleteTextures(1, &brdf_texture_id);

    delete depth_mesh_program;
    delete depth_skinned_mesh_program;
    delete depth_cube_mesh_program;
    delete depth_cube_skinned_mesh_program;
    delete color_program;
    delete geometry_mesh_program;
    delete geometry_skinned_mesh_program;
    delete geometry_terrain_program;
    delete deferred_ambient_program;
    delete deferred_directional_program;
    delete deferred_point_program;
    delete deferred_spot_program;
    delete skybox_program;
    delete water_program;
    delete sprite_program;
    delete gaussian_program;
    delete screen_program;

    delete water_dudv_texture;
    delete water_normal_texture;

    delete DEBUG_sphere_mesh;
}

void liminal::renderer::set_screen_size(GLsizei display_width, GLsizei display_height, float render_scale)
{
    this->display_width = display_width;
    this->display_height = display_height;
    render_width = (GLsizei)(display_width * render_scale);
    render_height = (GLsizei)(display_height * render_scale);

    glDeleteFramebuffers(1, &geometry_fbo_id);
    glDeleteTextures(1, &geometry_position_texture_id);
    glDeleteTextures(1, &geometry_normal_texture_id);
    glDeleteTextures(1, &geometry_albedo_texture_id);
    glDeleteTextures(1, &geometry_material_texture_id);
    glDeleteRenderbuffers(1, &geometry_rbo_id);

    glDeleteFramebuffers(1, &hdr_fbo_id);
    glDeleteTextures(2, hdr_texture_ids);
    glDeleteRenderbuffers(1, &hdr_rbo_id);

    glDeleteFramebuffers(2, bloom_fbo_ids);
    glDeleteTextures(2, bloom_texture_ids);

    // setup geometry fbo
    // gbuffer:
    //      position - rgb16f
    //      normal - rgb16f
    //      albedo - rgb16f
    //      material - rgba16f
    //          metallic - r
    //          roughness - g
    //          occlusion - b
    //          height - a
    glGenFramebuffers(1, &geometry_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, geometry_fbo_id);
    {
        {
            glGenTextures(1, &geometry_position_texture_id);
            glBindTexture(GL_TEXTURE_2D, geometry_position_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB16F,
                    render_width,
                    render_height,
                    0,
                    GL_RGB,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                geometry_position_texture_id,
                0);
        }

        {
            glGenTextures(1, &geometry_normal_texture_id);
            glBindTexture(GL_TEXTURE_2D, geometry_normal_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB16F,
                    render_width,
                    render_height,
                    0,
                    GL_RGB,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT1,
                GL_TEXTURE_2D,
                geometry_normal_texture_id,
                0);
        }

        {
            glGenTextures(1, &geometry_albedo_texture_id);
            glBindTexture(GL_TEXTURE_2D, geometry_albedo_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB16F,
                    render_width,
                    render_height,
                    0,
                    GL_RGB,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT2,
                GL_TEXTURE_2D,
                geometry_albedo_texture_id,
                0);
        }

        {
            glGenTextures(1, &geometry_material_texture_id);
            glBindTexture(GL_TEXTURE_2D, geometry_material_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA16F,
                    render_width,
                    render_height,
                    0,
                    GL_RGBA,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT3,
                GL_TEXTURE_2D,
                geometry_material_texture_id,
                0);
        }

        {
            glGenRenderbuffers(1, &geometry_rbo_id);
            glBindRenderbuffer(GL_RENDERBUFFER, geometry_rbo_id);
            {
                glRenderbufferStorage(
                    GL_RENDERBUFFER,
                    GL_DEPTH_COMPONENT,
                    render_width,
                    render_height);
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER,
                geometry_rbo_id);
        }

        {
            GLenum geometry_color_attachments[] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3};
            glDrawBuffers(sizeof(geometry_color_attachments) / sizeof(GLenum), geometry_color_attachments);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: Failed to create geometry framebuffer" << std::endl;
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // setup hdr fbo
    glGenFramebuffers(1, &hdr_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_id);
    {
        {
            glGenTextures(2, hdr_texture_ids);
            for (unsigned int i = 0; i < 2; i++)
            {
                glBindTexture(GL_TEXTURE_2D, hdr_texture_ids[i]);
                {
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RGBA16F,
                        render_width,
                        render_height,
                        0,
                        GL_RGBA,
                        GL_FLOAT,
                        nullptr);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                glBindTexture(GL_TEXTURE_2D, 0);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0 + i,
                    GL_TEXTURE_2D,
                    hdr_texture_ids[i],
                    0);
            }
        }

        {
            glGenRenderbuffers(1, &hdr_rbo_id);
            glBindRenderbuffer(GL_RENDERBUFFER, hdr_rbo_id);
            {
                glRenderbufferStorage(
                    GL_RENDERBUFFER,
                    GL_DEPTH_STENCIL,
                    display_width,
                    display_height);
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER,
                hdr_rbo_id);
        }

        {
            GLenum hdr_color_attachments[] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1};
            glDrawBuffers(sizeof(hdr_color_attachments) / sizeof(GLenum), hdr_color_attachments);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: Failed to create hdr framebuffer" << std::endl;
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // setup bloom fbo
    glGenFramebuffers(2, bloom_fbo_ids);
    glGenTextures(2, bloom_texture_ids);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo_ids[i]);
        {
            {
                glBindTexture(GL_TEXTURE_2D, bloom_texture_ids[i]);
                {
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RGBA16F,
                        display_width / 8,
                        display_height / 8,
                        0,
                        GL_RGBA,
                        GL_FLOAT,
                        NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                glBindTexture(GL_TEXTURE_2D, 0);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D,
                    bloom_texture_ids[i],
                    0);
            }

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::cerr << "Error: Failed to create bloom framebuffer" << std::endl;
                return;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void liminal::renderer::set_reflection_size(GLsizei reflection_width, GLsizei reflection_height)
{
    this->reflection_width = reflection_width;
    this->reflection_height = reflection_height;

    glDeleteFramebuffers(1, &water_reflection_fbo_id);
    glDeleteTextures(1, &water_reflection_color_texture_id);
    glDeleteRenderbuffers(1, &water_reflection_rbo_id);

    // setup water reflection fbo
    glGenFramebuffers(1, &water_reflection_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, water_reflection_fbo_id);
    {
        {
            glGenTextures(1, &water_reflection_color_texture_id);
            glBindTexture(GL_TEXTURE_2D, water_reflection_color_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA16F,
                    reflection_width,
                    reflection_height,
                    0,
                    GL_RGBA,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                water_reflection_color_texture_id,
                0);
        }

        {
            glGenRenderbuffers(1, &water_reflection_rbo_id);
            glBindRenderbuffer(GL_RENDERBUFFER, water_reflection_rbo_id);
            {
                glRenderbufferStorage(
                    GL_RENDERBUFFER,
                    GL_DEPTH_COMPONENT,
                    reflection_width,
                    reflection_height);
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER,
                water_reflection_rbo_id);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: Failed to create water reflection framebuffer" << std::endl;
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void liminal::renderer::set_refraction_size(GLsizei refraction_width, GLsizei refraction_height)
{
    this->refraction_width = refraction_width;
    this->refraction_height = refraction_height;

    glDeleteFramebuffers(1, &water_refraction_fbo_id);
    glDeleteTextures(1, &water_refraction_color_texture_id);
    glDeleteTextures(1, &water_refraction_depth_texture_id);

    // setup water refraction fbo
    glGenFramebuffers(1, &water_refraction_fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, water_refraction_fbo_id);
    {
        {
            glGenTextures(1, &water_refraction_color_texture_id);
            glBindTexture(GL_TEXTURE_2D, water_refraction_color_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA16F,
                    refraction_width,
                    refraction_height,
                    0,
                    GL_RGBA,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                water_refraction_color_texture_id,
                0);
        }

        {
            glGenTextures(1, &water_refraction_depth_texture_id);
            glBindTexture(GL_TEXTURE_2D, water_refraction_depth_texture_id);
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_DEPTH_COMPONENT,
                    refraction_width,
                    refraction_height,
                    0,
                    GL_DEPTH_COMPONENT,
                    GL_FLOAT,
                    nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_TEXTURE_2D,
                water_refraction_depth_texture_id,
                0);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: Failed to create water refraction framebuffer" << std::endl;
            return;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void liminal::renderer::reload_programs()
{
    depth_mesh_program->reload();
    depth_skinned_mesh_program->reload();
    depth_cube_mesh_program->reload();
    depth_cube_skinned_mesh_program->reload();
    color_program->reload();
    geometry_mesh_program->reload();
    geometry_skinned_mesh_program->reload();
    geometry_terrain_program->reload();
    deferred_ambient_program->reload();
    deferred_directional_program->reload();
    deferred_point_program->reload();
    deferred_spot_program->reload();
    skybox_program->reload();
    water_program->reload();
    sprite_program->reload();
    gaussian_program->reload();
    screen_program->reload();

    setup_samplers();
}

void liminal::renderer::setup_samplers()
{
    geometry_mesh_program->bind();
    {
        geometry_mesh_program->set_int("material.albedo_map", 0);
        geometry_mesh_program->set_int("material.normal_map", 1);
        geometry_mesh_program->set_int("material.metallic_map", 2);
        geometry_mesh_program->set_int("material.roughness_map", 3);
        geometry_mesh_program->set_int("material.occlusion_map", 4);
        geometry_mesh_program->set_int("material.height_map", 5);
    }
    geometry_mesh_program->unbind();

    geometry_skinned_mesh_program->bind();
    {
        geometry_skinned_mesh_program->set_int("material.albedo_map", 0);
        geometry_skinned_mesh_program->set_int("material.normal_map", 1);
        geometry_skinned_mesh_program->set_int("material.metallic_map", 2);
        geometry_skinned_mesh_program->set_int("material.roughness_map", 3);
        geometry_skinned_mesh_program->set_int("material.occlusion_map", 4);
        geometry_skinned_mesh_program->set_int("material.height_map", 5);
    }
    geometry_skinned_mesh_program->unbind();

    geometry_terrain_program->bind();
    {
        geometry_terrain_program->set_int("materials[0].albedo_map", 0);
        geometry_terrain_program->set_int("materials[0].normal_map", 1);
        geometry_terrain_program->set_int("materials[0].metallic_map", 2);
        geometry_terrain_program->set_int("materials[0].roughness_map", 3);
        geometry_terrain_program->set_int("materials[0].occlusion_map", 4);
        geometry_terrain_program->set_int("materials[0].height_map", 5);
    }
    geometry_terrain_program->unbind();

    deferred_ambient_program->bind();
    {
        deferred_ambient_program->set_int("geometry.position_map", 0);
        deferred_ambient_program->set_int("geometry.normal_map", 1);
        deferred_ambient_program->set_int("geometry.albedo_map", 2);
        deferred_ambient_program->set_int("geometry.material_map", 3);
        deferred_ambient_program->set_int("skybox.irradiance_cubemap", 4);
        deferred_ambient_program->set_int("skybox.prefilter_cubemap", 5);
        deferred_ambient_program->set_int("brdf_map", 6);
    }
    deferred_ambient_program->unbind();

    deferred_directional_program->bind();
    {
        deferred_directional_program->set_int("geometry.position_map", 0);
        deferred_directional_program->set_int("geometry.normal_map", 1);
        deferred_directional_program->set_int("geometry.albedo_map", 2);
        deferred_directional_program->set_int("geometry.material_map", 3);
        deferred_directional_program->set_int("light.depth_map", 4);
    }
    deferred_directional_program->unbind();

    deferred_point_program->bind();
    {
        deferred_point_program->set_int("geometry.position_map", 0);
        deferred_point_program->set_int("geometry.normal_map", 1);
        deferred_point_program->set_int("geometry.albedo_map", 2);
        deferred_point_program->set_int("geometry.material_map", 3);
        deferred_point_program->set_int("light.depth_cubemap", 4);
    }
    deferred_point_program->unbind();

    deferred_spot_program->bind();
    {
        deferred_spot_program->set_int("geometry.position_map", 0);
        deferred_spot_program->set_int("geometry.normal_map", 1);
        deferred_spot_program->set_int("geometry.albedo_map", 2);
        deferred_spot_program->set_int("geometry.material_map", 3);
        deferred_spot_program->set_int("light.depth_map", 4);
    }
    deferred_spot_program->unbind();

    skybox_program->bind();
    {
        skybox_program->set_int("skybox.environment_cubemap", 0);
    }
    skybox_program->unbind();

    water_program->bind();
    {
        water_program->set_int("water.reflection_map", 0);
        water_program->set_int("water.refraction_map", 1);
        water_program->set_int("water.depth_map", 2);
        water_program->set_int("water.dudv_map", 3);
        water_program->set_int("water.normal_map", 4);
    }
    water_program->unbind();

    sprite_program->bind();
    {
        sprite_program->set_int("sprite.texture", 0);
    }
    sprite_program->unbind();

    gaussian_program->bind();
    {
        gaussian_program->set_int("image", 0);
    }
    gaussian_program->unbind();

    screen_program->bind();
    {
        screen_program->set_int("hdr_map", 0);
        screen_program->set_int("bloom_map", 1);
    }
    screen_program->unbind();
}

void liminal::renderer::flush(unsigned int current_time, float delta_time)
{
    if (!camera)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    // update animations
    for (auto &object : objects)
    {
        if (object->model->has_animations())
        {
            object->model->update_bone_transformations(current_time);
        }
    }

    // render everything
    render_shadows();
    render_objects(hdr_fbo_id, render_width, render_height);
    if (waters.size() > 0)
    {
        render_waters(current_time);
    }
    if (sprites.size() > 0)
    {
        render_sprites();
    }
    render_screen();

    // reset render state
    wireframe = false;
    greyscale = false;
    camera = nullptr;
    skybox = nullptr;
    objects.clear();
    directional_lights.clear();
    point_lights.clear();
    spot_lights.clear();
    waters.clear();
    terrains.clear();
    sprites.clear();
}

void liminal::renderer::render_shadows()
{
    for (auto &directional_light : directional_lights)
    {
        directional_light->update_transformation_matrix(camera->position);

        for (unsigned int i = 0; i < NUM_CASCADES; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, directional_light->depth_map_fbo_id);
            {
                glViewport(0, 0, directional_light->depth_map_size, directional_light->depth_map_size);
                glEnable(GL_CULL_FACE);

                glFramebufferTexture2D(
                    GL_FRAMEBUFFER,
                    GL_DEPTH_ATTACHMENT,
                    GL_TEXTURE_2D,
                    directional_light->depth_map_texture_ids[i],
                    0);

                glClear(GL_DEPTH_BUFFER_BIT);

                for (auto &object : objects)
                {
                    glm::mat4 object_model = object->calc_model();
                    if (object->model->has_animations())
                    {
                        depth_skinned_mesh_program->bind();
                        {
                            depth_skinned_mesh_program->set_mat4("mvp", directional_light->transformation_matrix * object_model);
                            depth_skinned_mesh_program->set_mat4_vector("bone_transformations", object->model->bone_transformations);

                            object->model->draw_meshes(depth_skinned_mesh_program);
                        }
                        depth_skinned_mesh_program->unbind();
                    }
                    else
                    {
                        depth_mesh_program->bind();
                        {
                            depth_mesh_program->set_mat4("mvp", directional_light->transformation_matrix * object_model);

                            object->model->draw_meshes(depth_mesh_program);
                        }
                        depth_mesh_program->unbind();
                    }
                }

                depth_mesh_program->bind();
                {

                    for (auto &terrain : terrains)
                    {
                        glm::mat4 terrain_model = terrain->calc_model();

                        depth_mesh_program->set_mat4("mvp", directional_light->transformation_matrix * terrain_model);

                        terrain->mesh->draw(depth_mesh_program);
                    }
                }
                depth_mesh_program->unbind();
            }

            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    for (auto &point_light : point_lights)
    {
        point_light->update_transformation_matrices();

        glBindFramebuffer(GL_FRAMEBUFFER, point_light->depth_cubemap_fbo_id);
        {
            glViewport(0, 0, point_light->depth_cube_size, point_light->depth_cube_size);
            glEnable(GL_CULL_FACE);

            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto &object : objects)
            {
                glm::mat4 object_model = object->calc_model();
                if (object->model->has_animations())
                {
                    depth_cube_skinned_mesh_program->bind();
                    {
                        depth_cube_skinned_mesh_program->set_mat4("model", object_model);
                        depth_cube_skinned_mesh_program->set_mat4_vector("bone_transformations", object->model->bone_transformations);

                        for (unsigned int i = 0; i < 6; i++)
                        {
                            depth_cube_skinned_mesh_program->set_mat4("light.transformation_matrices[" + std::to_string(i) + "]", point_light->transformation_matrices[i]);
                        }

                        depth_cube_skinned_mesh_program->set_float("light.far_plane", point_light::far_plane);
                        depth_cube_skinned_mesh_program->set_vec3("light.position", point_light->position);

                        object->model->draw_meshes(depth_cube_skinned_mesh_program);
                    }
                    depth_cube_skinned_mesh_program->unbind();
                }
                else
                {
                    depth_cube_mesh_program->bind();
                    {
                        depth_cube_mesh_program->set_mat4("model", object_model);

                        for (unsigned int i = 0; i < 6; i++)
                        {
                            depth_cube_mesh_program->set_mat4("light.transformation_matrices[" + std::to_string(i) + "]", point_light->transformation_matrices[i]);
                        }

                        depth_cube_mesh_program->set_float("light.far_plane", point_light::far_plane);
                        depth_cube_mesh_program->set_vec3("light.position", point_light->position);

                        object->model->draw_meshes(depth_cube_mesh_program);
                    }
                    depth_cube_mesh_program->unbind();
                }
            }

            depth_cube_mesh_program->bind();
            {
                for (unsigned int i = 0; i < 6; i++)
                {
                    depth_cube_mesh_program->set_mat4("light.transformation_matrices[" + std::to_string(i) + "]", point_light->transformation_matrices[i]);
                }

                depth_cube_mesh_program->set_float("light.far_plane", point_light::far_plane);
                depth_cube_mesh_program->set_vec3("light.position", point_light->position);

                for (auto &terrain : terrains)
                {
                    glm::mat4 terrain_model = terrain->calc_model();

                    depth_cube_mesh_program->set_mat4("model", terrain_model);

                    terrain->mesh->draw(depth_cube_mesh_program);
                }
            }
            depth_cube_mesh_program->unbind();

            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    for (auto &spot_light : spot_lights)
    {
        spot_light->update_transformation_matrix();

        glBindFramebuffer(GL_FRAMEBUFFER, spot_light->depth_map_fbo_id);
        {
            glViewport(0, 0, spot_light->depth_map_size, spot_light->depth_map_size);
            glEnable(GL_CULL_FACE);

            glClear(GL_DEPTH_BUFFER_BIT);

            for (auto &object : objects)
            {
                glm::mat4 object_model = object->calc_model();
                if (object->model->has_animations())
                {
                    depth_skinned_mesh_program->bind();
                    {
                        depth_skinned_mesh_program->set_mat4("mvp", spot_light->transformation_matrix * object_model);
                        depth_skinned_mesh_program->set_mat4_vector("bone_transformations", object->model->bone_transformations);

                        object->model->draw_meshes(depth_skinned_mesh_program);
                    }
                    depth_skinned_mesh_program->unbind();
                }
                else
                {
                    depth_mesh_program->bind();
                    {
                        depth_mesh_program->set_mat4("mvp", spot_light->transformation_matrix * object_model);

                        object->model->draw_meshes(depth_mesh_program);
                    }
                    depth_mesh_program->unbind();
                }
            }

            depth_mesh_program->bind();
            {

                for (auto &terrain : terrains)
                {
                    glm::mat4 terrain_model = terrain->calc_model();

                    depth_mesh_program->set_mat4("mvp", spot_light->transformation_matrix * terrain_model);

                    terrain->mesh->draw(depth_mesh_program);
                }
            }
            depth_mesh_program->unbind();

            glDisable(GL_CULL_FACE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void liminal::renderer::render_objects(GLuint fbo_id, GLsizei width, GLsizei height, glm::vec4 clipping_plane)
{
    // camera
    glm::mat4 camera_projection = camera->calc_projection((float)width / (float)height);
    glm::mat4 camera_view = camera->calc_view();

    // draw to gbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, geometry_fbo_id);
    {
        glViewport(0, 0, width, height);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glEnable(GL_CULL_FACE);
        glEnable(GL_CLIP_DISTANCE0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &object : objects)
        {
            glm::mat4 object_model = object->calc_model();
            if (object->model->has_animations())
            {
                geometry_skinned_mesh_program->bind();
                {
                    geometry_skinned_mesh_program->set_mat4("mvp", camera_projection * camera_view * object_model);
                    geometry_skinned_mesh_program->set_mat4_vector("bone_transformations", object->model->bone_transformations);
                    geometry_skinned_mesh_program->set_mat4("model", object_model);
                    geometry_skinned_mesh_program->set_vec4("clipping_plane", clipping_plane);

                    object->model->draw_meshes(geometry_skinned_mesh_program);
                }
                geometry_skinned_mesh_program->unbind();
            }
            else
            {
                geometry_mesh_program->bind();
                {
                    geometry_mesh_program->set_mat4("mvp", camera_projection * camera_view * object_model);
                    geometry_mesh_program->set_mat4("model", object_model);
                    geometry_mesh_program->set_vec4("clipping_plane", clipping_plane);

                    object->model->draw_meshes(geometry_mesh_program);
                }
                geometry_mesh_program->unbind();
            }
        }

        geometry_terrain_program->bind();
        {
            geometry_terrain_program->set_vec4("clipping_plane", clipping_plane);

            for (auto &terrain : terrains)
            {
                glm::mat4 terrain_model = terrain->calc_model();

                geometry_terrain_program->set_mat4("mvp", camera_projection * camera_view * terrain_model);
                geometry_terrain_program->set_mat4("model", terrain_model);
                geometry_terrain_program->set_float("tiling", terrain->size);

                terrain->mesh->draw(geometry_terrain_program);
            }
        }
        geometry_terrain_program->unbind();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);
        glDisable(GL_CLIP_DISTANCE0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // deferred lighting
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    {
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // IBL
        deferred_ambient_program->bind();
        {
            deferred_ambient_program->set_vec3("camera.position", camera->position);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, geometry_position_texture_id);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, geometry_normal_texture_id);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, geometry_albedo_texture_id);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, geometry_material_texture_id);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox ? skybox->irradiance_cubemap_id : 0);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox ? skybox->prefilter_cubemap_id : 0);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, brdf_texture_id);

            glBindVertexArray(screen_vao_id);
            glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        deferred_ambient_program->unbind();

        // blend the rest of the lights
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);

            if (directional_lights.size() > 0)
            {
                deferred_directional_program->bind();
                {
                    deferred_directional_program->set_vec3("camera.position", camera->position);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, geometry_position_texture_id);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, geometry_normal_texture_id);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, geometry_albedo_texture_id);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, geometry_material_texture_id);

                    for (auto &directional_light : directional_lights)
                    {
                        deferred_directional_program->set_vec3("light.direction", directional_light->direction);
                        deferred_directional_program->set_vec3("light.color", directional_light->color);
                        deferred_directional_program->set_mat4("light.transformation_matrix", directional_light->transformation_matrix);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, directional_light->depth_map_texture_ids[0]);

                        glBindVertexArray(screen_vao_id);
                        glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
                        glBindVertexArray(0);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                deferred_directional_program->unbind();
            }

            if (point_lights.size() > 0)
            {
                deferred_point_program->bind();
                {
                    deferred_point_program->set_vec3("camera.position", camera->position);
                    deferred_point_program->set_float("light.far_plane", point_light::far_plane);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, geometry_position_texture_id);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, geometry_normal_texture_id);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, geometry_albedo_texture_id);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, geometry_material_texture_id);

                    for (auto &point_light : point_lights)
                    {
                        deferred_point_program->set_vec3("light.position", point_light->position);
                        deferred_point_program->set_vec3("light.color", point_light->color);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, point_light->depth_cubemap_texture_id);

                        glBindVertexArray(screen_vao_id);
                        glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
                        glBindVertexArray(0);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                    }

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                deferred_point_program->unbind();
            }

            if (spot_lights.size() > 0)
            {
                deferred_spot_program->bind();
                {
                    deferred_spot_program->set_vec3("camera.position", camera->position);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, geometry_position_texture_id);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, geometry_normal_texture_id);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, geometry_albedo_texture_id);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, geometry_material_texture_id);

                    for (auto &spot_light : spot_lights)
                    {
                        deferred_spot_program->set_vec3("light.position", spot_light->position);
                        deferred_spot_program->set_vec3("light.direction", spot_light->direction);
                        deferred_spot_program->set_vec3("light.color", spot_light->color);
                        deferred_spot_program->set_float("light.inner_cutoff", spot_light->inner_cutoff);
                        deferred_spot_program->set_float("light.outer_cutoff", spot_light->outer_cutoff);
                        deferred_spot_program->set_mat4("light.transformation_matrix", spot_light->transformation_matrix);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, spot_light->depth_map_texture_id);

                        glBindVertexArray(screen_vao_id);
                        glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
                        glBindVertexArray(0);

                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                deferred_spot_program->unbind();
            }

            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ZERO);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        glEnable(GL_DEPTH_TEST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // copy depth info
    glBindFramebuffer(GL_READ_FRAMEBUFFER, geometry_fbo_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // forward render everything else
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    {
        glViewport(0, 0, width, height);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

        // draw skybox
        if (skybox)
        {
            glDepthFunc(GL_LEQUAL);

            glm::mat4 camera_view_no_translate = camera_view;
            camera_view_no_translate[3][0] = 0.0f;
            camera_view_no_translate[3][1] = 0.0f;
            camera_view_no_translate[3][2] = 0.0f;

            skybox_program->bind();
            {
                skybox_program->set_mat4("mvp", camera_projection * camera_view_no_translate);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->environment_cubemap_id);

                glBindVertexArray(skybox_vao_id);
                glDrawArrays(GL_TRIANGLES, 0, skybox_vertices_size);
                glBindVertexArray(0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            }
            skybox_program->unbind();

            glDepthFunc(GL_LESS);
        }

        // DEBUG: draw point lights as a sphere
        {
            glEnable(GL_CLIP_DISTANCE0);

            for (auto &point_light : point_lights)
            {
                color_program->bind();
                {
                    glm::mat4 model(1.0f);
                    model = glm::translate(model, point_light->position);
                    model = glm::scale(model, {0.25f, 0.25f, 0.25f});

                    color_program->set_mat4("mvp", camera_projection * camera_view * model);
                    color_program->set_mat4("model", model);
                    color_program->set_vec4("clipping_plane", clipping_plane);
                    color_program->set_vec3("color", point_light->color);

                    DEBUG_sphere_mesh->draw(color_program);
                }
                color_program->unbind();
            }

            glDisable(GL_CLIP_DISTANCE0);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void liminal::renderer::render_waters(unsigned int current_time)
{
    for (auto &water : waters)
    {
        // reflection
        glm::vec4 reflection_clipping_plane = {0.0f, 1.0f, 0.0f, -water->position.y};
        if (camera->position.y < water->position.y) // flip reflection clipping plane if under the water
        {
            reflection_clipping_plane *= -1;
        }
        float previous_camera_y = camera->position.y;
        float previous_camera_pitch = camera->pitch;
        float previous_camera_roll = camera->roll;
        camera->position.y -= 2 * (camera->position.y - water->position.y);
        camera->pitch = -camera->pitch;
        camera->roll = -camera->roll;
        render_objects(water_reflection_fbo_id, reflection_width, reflection_height, reflection_clipping_plane);
        camera->position.y = previous_camera_y;
        camera->pitch = previous_camera_pitch;
        camera->roll = previous_camera_roll;

        // refraction
        glm::vec4 refraction_clipping_plane = {0.0f, -1.0f, 0.0f, water->position.y};
        if (camera->position.y < water->position.y) // flip refraction clipping plane if under the water
        {
            refraction_clipping_plane *= -1;
        }
        render_objects(water_refraction_fbo_id, refraction_width, refraction_height, refraction_clipping_plane);

        // draw water meshes
        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_id);
        {
            glViewport(0, 0, render_width, render_height);
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            water_program->bind();
            {
                glm::mat4 camera_projection = camera->calc_projection((float)render_width / (float)render_height);
                glm::mat4 camera_view = camera->calc_view();

                glm::mat4 water_model = water->calc_model();

                water_program->set_mat4("mvp", camera_projection * camera_view * water_model);
                water_program->set_mat4("model", water_model);
                water_program->set_float("tiling", water->size / 10);
                water_program->set_float("camera.near_plane", camera::near_plane);
                water_program->set_float("camera.far_plane", camera::far_plane);
                water_program->set_vec3("camera.position", camera->position);
                if (directional_lights.size() > 0)
                {
                    // TODO: specular reflections for all lights
                    water_program->set_vec3("light.direction", directional_lights[0]->direction);
                    water_program->set_vec3("light.color", directional_lights[0]->color);
                }
                water_program->set_unsigned_int("current_time", current_time);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, water_reflection_color_texture_id);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, water_refraction_color_texture_id);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, water_refraction_depth_texture_id);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, water_dudv_texture ? water_dudv_texture->texture_id : 0);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, water_normal_texture ? water_normal_texture->texture_id : 0);

                glBindVertexArray(water_vao_id);
                glDrawArrays(GL_TRIANGLES, 0, water_vertices_size);
                glBindVertexArray(0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, 0);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            water_program->unbind();

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ZERO);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void liminal::renderer::render_sprites()
{
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_id);
    {
        glViewport(0, 0, display_width, display_height);

        sprite_program->bind();
        {
            glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f);

            for (auto &sprite : sprites)
            {
                glm::mat4 sprite_model = sprite->calc_model();
                sprite_program->set_mat4("mvp", projection * sprite_model);
                sprite_program->set_vec3("sprite.color", sprite->color);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sprite->texture->texture_id);

                glBindVertexArray(sprite_vao_id);
                glDrawArrays(GL_TRIANGLES, 0, sprite_vertices_size);
                glBindVertexArray(0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
        sprite_program->unbind();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void liminal::renderer::render_screen()
{
    // apply gaussian blur to brightness map
    bool horizontal = true;
    {
        glViewport(0, 0, display_width / 8, display_height / 8);

        gaussian_program->bind();
        {
            bool first_iteration = true;
            for (unsigned int i = 0; i < 10; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, bloom_fbo_ids[horizontal]);
                {
                    gaussian_program->set_int("horizontal", horizontal);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, first_iteration ? hdr_texture_ids[1] : bloom_texture_ids[!horizontal]);

                    glBindVertexArray(screen_vao_id);
                    glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
                    glBindVertexArray(0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                horizontal = !horizontal;

                first_iteration = false;
            }
        }
        gaussian_program->unbind();
    }

    // final pass
    {
        glViewport(0, 0, display_width, display_height);
        glDisable(GL_DEPTH_TEST);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        screen_program->bind();
        {
            screen_program->set_unsigned_int("greyscale", greyscale);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hdr_texture_ids[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, bloom_texture_ids[!horizontal]);

            glBindVertexArray(screen_vao_id);
            glDrawArrays(GL_TRIANGLES, 0, screen_vertices_size);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        screen_program->unbind();

        glEnable(GL_DEPTH_TEST);
    }

    // DEBUG: draw fbos
    // {
    //     glViewport(0, 0, display_width, display_height);

    //     glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f);

    //     sprite_program->bind();
    //     {
    //         sprite_program->set_vec3("sprite.color", glm::vec3(1.0f, 1.0f, 1.0f));
    //         sprite_program->set_int("sprite.texture", 0);

    //         glm::mat4 model = glm::identity<glm::mat4>();
    //         model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

    //         {
    //             model = glm::translate(model, glm::vec3(-2.0f, 1.0f, 0.0f));

    //             sprite_program->set_mat4("mvp", projection * model);

    //             glActiveTexture(GL_TEXTURE0);
    //             glBindTexture(GL_TEXTURE_2D, directional_lights[0]->depth_map_texture_id);

    //             glBindVertexArray(sprite_vao_id);
    //             glDrawArrays(GL_TRIANGLES, 0, sprite_vertices_size);
    //             glBindVertexArray(0);
    //         }

    //         glActiveTexture(GL_TEXTURE0);
    //         glBindTexture(GL_TEXTURE_2D, 0);
    //     }
    //     sprite_program->unbind();
    // }
}
