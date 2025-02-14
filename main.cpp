/**
* Author: [Your name here]
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

//constexpr float BG_RED = 0.1922f,
//BG_BLUE = 0.549f,
//BG_GREEN = 0.9059f,
//BG_OPACITY = 1.0f;

// background: light purple
constexpr float BG_RED = 0.902f,
BG_BLUE = 0.745f,
BG_GREEN = 1.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr int TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4,
TRIANGLE_GREEN = 0.4,
TRIANGLE_OPACITY = 1.0;

SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;

int  g_frame_counter = 0;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix,
g_model_matrix1,
g_model_matrix2,
g_projection_matrix;

// ——————————— GLOBAL VARS AND CONSTS FOR TRANSFORMATIONS ——————————— //
float g_previous_ticks = 0.0f;
float g_orbit_angle = 0.0f;

const float ORBIT_RADIUS = 1.5f;
const float ORBIT_SPEED = 1.0f;

float ROT_ANGLE = 0.0f;
// —————————————————————————————————————————————————————————————————— //

// ——————————— TEXTURE ——————————— //
constexpr char OBJECT_ONE_FILEPATH[] = "assets/blue_kitty.png";
constexpr char OBJECT_TWO_FILEPATH[] = "assets/pink_kitty.png";

constexpr GLint NUMBER_OF_TEXTURES = 1; // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL = 0; // mipmap reduction image level
constexpr GLint TEXTURE_BORDER = 0; // this value MUST be zero

GLuint g_obj1_texture_id;
GLuint g_obj2_texture_id;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Kristie Assignment1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_model_matrix1 = glm::mat4(1.0f);
    g_model_matrix2 = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    g_shader_program.set_colour(TRIANGLE_RED,
        TRIANGLE_BLUE,
        TRIANGLE_GREEN,
        TRIANGLE_OPACITY);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED,
        BG_BLUE,
        BG_GREEN,
        BG_OPACITY);

    g_obj1_texture_id = load_texture(OBJECT_ONE_FILEPATH);
    g_obj2_texture_id = load_texture(OBJECT_TWO_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    // ——————————— delta time ——————————— //
    float g_ticks = (float)SDL_GetTicks() / 1000.0f;  // get the current number of g_ticks
    float g_delta_time = g_ticks - g_previous_ticks;       // the delta time is the difference from the last frame
    g_previous_ticks = g_ticks;

    // ——————————— orbit transformation ——————————— //
    g_orbit_angle += ORBIT_SPEED * g_delta_time;

    float x = ORBIT_RADIUS * sin(g_orbit_angle);
    float y = ORBIT_RADIUS * cos(g_orbit_angle);

    ROT_ANGLE += glm::radians(45.0f) * g_delta_time;

    // ——————————— scaling (periodic) ——————————— //
    float scale_factor = 1.0f + 0.5f * sin(g_ticks * 2.0f); // Periodic scaling

    glm::vec3 scale_vector;
    scale_vector = glm::vec3(scale_factor, scale_factor, 1.0f);

    g_model_matrix1 = glm::mat4(1.0f);
    g_model_matrix1 = glm::scale(g_model_matrix1, scale_vector);
    g_model_matrix1 = glm::translate(g_model_matrix1, glm::vec3(x, y, 0.0f));
    g_model_matrix1 = glm::rotate(g_model_matrix1, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));

    g_model_matrix2 = glm::translate(g_model_matrix1, glm::vec3(y, x, 0.0f));
    g_model_matrix2 = glm::rotate(g_model_matrix2, ROT_ANGLE, glm::vec3(0.0f, 1.0f, 0.0f));

}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); //2 triangles -> 6, not 3
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    g_shader_program.set_model_matrix(g_model_matrix1);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };


    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_model_matrix1, g_obj1_texture_id);
    draw_object(g_model_matrix2, g_obj2_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}