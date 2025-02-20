/**
* Author: Hailee Yun
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// note to self: have 2 sprites of my own art, rotating on the z-axis.
// successfully runs


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'


#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               

#define STB_IMAGE_IMPLEMENTATION // to load my own images
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };
enum ScaleDirection { GROWING, SHRINKING };


// Our window dimensions
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

// Background color components
constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

// shaders
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";


// texture constants
constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

constexpr char BLACK_CAT_SPRITE_FILEPATH[]   = "shaders/black_cat.png",
               BUTTERFLY_SPRITE_FILEPATH[] = "shaders/butterfly.png";

// initial position and scale of sprites
constexpr glm::vec3 INIT_SCALE      = glm::vec3(2.0f, 2.0f, 0.0f),
                    INIT_SCALE_BLACK_CAT = glm::vec3(2.0f, 2.0f, 0.0f),
                    INIT_SCALE_BUTTERFLY = glm::vec3(1.0f, 1.0f, 0.0f),
                    INIT_POS_BLACK_CAT   = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_BUTTERFLY = glm::vec3(-2.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f; // rotational constant


// Our viewport�or our "camera"'s�position and dimensions
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;


// Our object's fill colour
constexpr float TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4f,
TRIANGLE_GREEN = 0.4f,
TRIANGLE_OPACITY = 1.0f;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
g_black_cat_matrix,
g_butterfly_matrix,
g_model_matrix,       // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.

constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
//glm::vec3 g_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_position_black_cat = glm::vec3(0.0f, 0.0f, 0.0f); // what is added for their positions
glm::vec3 g_position_butterfly = glm::vec3(0.0f, 0.0f, 0.0f);
float g_previous_ticks;
float g_radius = 2;
float g_frames = 0.0f;
float g_butterfly_radius = 1;

int g_frame_counter = 0;
ScaleDirection g_scale_direction = GROWING;


// what is added for their rotations
glm::vec3 g_rotation_black_cat   = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_butterfly = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_scale_butterfly = glm::vec3(0.0f, 0.0f, 0.0f); // to be added to butterfly scale vector so that it pulses

constexpr float BASE_SCALE = 1.0f,      // The unscaled size of your object
MAX_AMPLITUDE = 0.4f,  // The most our triangle will be scaled up/down
PULSE_SPEED = 0.5f;    // How fast you want your triangle to "beat"
// the lower the pulse_speed, the faster it beats

constexpr float GROWTH_FACTOR = 1.1f; // not sure if i need these yet
constexpr float SHRINK_FACTOR = 0.9f;
constexpr int   MAX_FRAME = 40;


GLuint g_black_cat_texture_id,
       g_butterfly_texture_id;

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
    SDL_Init(SDL_INIT_VIDEO);  // initialize video

    g_display_window = SDL_CreateWindow("project 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

      

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;

        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_black_cat_matrix       = glm::mat4(1.0f);
    g_butterfly_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    //g_model_matrix = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_black_cat_texture_id   = load_texture(BLACK_CAT_SPRITE_FILEPATH);
    g_butterfly_texture_id = load_texture(BUTTERFLY_SPRITE_FILEPATH);

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
    g_frame_counter++;
    /* Delta time calculations */
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    // rotation the black cat and butterfly across the z-axis
    //g_rotation_black_cat.y += ROT_INCREMENT * delta_time;
    g_rotation_butterfly.y += -1 * ROT_INCREMENT * delta_time;

    // try to have the black cat circle the sceen
    static float rotation_angle = 0.0f;
    float rotation_speed = 1.0f;
    rotation_angle += rotation_speed * delta_time;
    g_position_black_cat.x = g_radius * cos(rotation_angle);
    g_position_black_cat.y = g_radius * sin(rotation_angle);

    // butterfly spins twice as fast
    g_position_butterfly.x = g_butterfly_radius * cos(rotation_angle*2);
    g_position_butterfly.y = g_butterfly_radius * sin(rotation_angle*2);
    

    // butterfly pulse
    float butterfly_scale_factor = BASE_SCALE + MAX_AMPLITUDE * glm::cos(rotation_angle / PULSE_SPEED);
    glm::vec3 butterfly_scale_factors = glm::vec3(butterfly_scale_factor, butterfly_scale_factor, 0.0f);

    /* Model matrix reset */
    g_black_cat_matrix   = glm::mat4(1.0f);
    g_butterfly_matrix = glm::mat4(1.0f);
    
    /* Transformations */
    g_black_cat_matrix = glm::translate(g_black_cat_matrix, g_position_black_cat);
    g_black_cat_matrix = glm::rotate(g_black_cat_matrix,
                                 g_rotation_black_cat.y,
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    g_black_cat_matrix = glm::scale(g_black_cat_matrix, INIT_SCALE_BLACK_CAT);
    
    g_butterfly_matrix = glm::translate(g_black_cat_matrix, g_position_butterfly);
    g_butterfly_matrix = glm::rotate(g_butterfly_matrix,
                                  g_rotation_butterfly.y,
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    g_butterfly_matrix = glm::scale(g_butterfly_matrix, butterfly_scale_factors);


}

void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

// binding and rendering the textures
void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
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
    draw_object(g_black_cat_matrix, g_black_cat_texture_id);
    draw_object(g_butterfly_matrix, g_butterfly_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }

/**
 Start here�we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program�whatever that means
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything�press a button, move the joystick�process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }

    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}
