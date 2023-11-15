#include <tracy/Tracy.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


enum class game_object
{
    PLAYER,
    TERRAIN,
    SLIME
};

struct entity
{
    game_object type;
    glm::vec2 scale;
    glm::vec2 position;
};

entity create_player_entity(float x, float y)
{
    entity PLAYER;
    PLAYER.type = game_object::PLAYER;
    PLAYER.scale = { 128, 128 };
    PLAYER.position = { x, y };
    return PLAYER;
}

entity create_slime_entity(float x, float y)
{
    return{ game_object::SLIME, {64, 64}, {x , y} };
}

namespace
{
  void GLAPIENTRY OpenglErrorCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    [[maybe_unused]] GLsizei length,
    const GLchar* message,
    [[maybe_unused]] const void* userParam)
  {
    // Ignore certain verbose info messages (particularly ones on Nvidia).
    if (id == 131169 ||
      id == 131185 || // NV: Buffer will use video memory
      id == 131218 ||
      id == 131204 || // Texture cannot be used for texture mapping
      id == 131222 ||
      id == 131154 || // NV: pixel transfer is synchronized with 3D rendering
      id == 131220 || // NV: A fragment shader is required to render to an integer framebuffer
      id == 131140 || // NV: Blending is enabled while an integer render texture is in the bound framebuffer
      id == 0         // gl{Push, Pop}DebugGroup
      )
      return;

    std::stringstream errStream;
    errStream << "OpenGL Debug message (" << id << "): " << message << '\n';

    switch (source)
    {
    case GL_DEBUG_SOURCE_API: errStream << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: errStream << "Source: Window Manager"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: errStream << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY: errStream << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION: errStream << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER: errStream << "Source: Other"; break;
    }

    errStream << '\n';

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: errStream << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errStream << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: errStream << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY: errStream << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE: errStream << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER: errStream << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP: errStream << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP: errStream << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER: errStream << "Type: Other"; break;
    }

    errStream << '\n';

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: errStream << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM: errStream << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW: errStream << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: errStream << "Severity: notification"; break;
    }

    std::cout << errStream.str() << '\n';
  }
} // namespace

// Square corrdinates
float vertices[] = {
    // positions          // colors           // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
};
unsigned int indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3,    // second triangle
};

const char* vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;
uniform mat4 projection;
uniform vec2 scale;
uniform vec2 position;

void main()
{
   gl_Position = projection * vec4(aPos * scale + position, 0.0, 1.0);
   ourColor = aColor;
   TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
   FragColor = texture(ourTexture, TexCoord);
}
)";

GLuint load_texture(const char* path) {
    // Load Texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // load and generate the texture
    int width, height, nrChannels;
    //stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 4);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    return(texture);
}



int main([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv)
{
  ZoneScoped; // Tells Tracy to profile this scope

  // Initialiize GLFW
  if (!glfwInit())
  {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwSetErrorCallback([](int, const char* desc) { std::cout << "GLFW error: " << desc << '\n'; });

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);

  // Create a monitor that is fullscreen
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (monitor == nullptr)
  {
    throw std::runtime_error("No monitor detected");
  }
  const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
  GLFWwindow* window = glfwCreateWindow(static_cast<int>(videoMode->width), static_cast<int>(videoMode->height), "bababooey", nullptr, nullptr);
  if (!window)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to create window");
  }

  // Position the window in the center of the screen
  int windowWidth{};
  int windowHeight{};
  glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
  int monitorLeft{};
  int monitorTop{};
  glfwGetMonitorPos(monitor, &monitorLeft, &monitorTop);
  glfwSetWindowPos(window, videoMode->width / 2 - windowWidth / 2 + monitorLeft, videoMode->height / 2 - windowHeight / 2 + monitorTop);
  
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // 1 = vsync, 0 = no vsync

  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, -1.0f, 1.0f);

  // Load OpenGL function pointers
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0)
  {
    glfwTerminate();
    throw std::runtime_error("Failed to initialize OpenGL");
  }

  // Set up the GL debug message callback. OpenglErrorCallback gets invoked whenever you make a goof with the API.
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenglErrorCallback, nullptr);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

  // Initialize ImGui and a backend for it.
  // Because we allow the GLFW backend to install callbacks, it will automatically call our own that we provided.
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();
  ImGui::StyleColorsDark();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  // Vertex buffer object
  unsigned int VBO;
  glGenBuffers(1, &VBO);

  // Vertex shader
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // Error checking
  int  success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

  if (!success)
  {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  // Fragment shader
  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  // Error checking
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
  }

  glUseProgram(shaderProgram);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  unsigned int EBO;
  glGenBuffers(1, &EBO);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  // ..:: Initialization code (done once (unless your object frequently changes)) :: ..
  // 1. bind Vertex Array Object
  glBindVertexArray(VAO);
  // 2. copy our vertices array in a buffer for OpenGL to use
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  // 3. copy our index array in a element buffer for OpenGL to use
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  // 4. then set the vertex attributes pointers
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLuint player_texture = load_texture("models/Sprite-0002.png");

  std::vector<entity> entities;
  entities.push_back(create_player_entity(static_cast<float>(windowWidth) / 2, static_cast<float>(windowHeight) / 2));
  //Game loop
  while(!glfwWindowShouldClose(window))
  {
    // Profile the main loop
    ZoneTransient(mainLoop, true);

    // Clear the window
    glClearColor(1.0f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Call these each frame for ImGui to work
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
      glfwSetWindowShouldClose(window, true);
    }



    // Create a window with ImGui and put some text in it
    ImGui::Begin("Test window");
    ImGui::Text("bababooey");
    ImGui::SliderFloat2("position", glm::value_ptr(entities[0].position), 0, static_cast<float>(windowWidth));
    ImGui::SliderFloat2("scale", glm::value_ptr(entities[0].scale), 0, 1000);
    ImGui::End();

    // Draw ImGui to the screen
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  
    // ..:: Drawing code (in render loop) :: ..
    // 4. draw the object
    glUseProgram(shaderProgram);
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(VAO);
    for (const auto& entity : entities)
    {
        if (entity.type == game_object::PLAYER)
        {
            glBindTexture(GL_TEXTURE_2D, player_texture);
        }
        int scaleLoc = glGetUniformLocation(shaderProgram, "scale");
        glUniform2fv(scaleLoc, 1, glm::value_ptr(entity.scale));
        int positionLoc = glGetUniformLocation(shaderProgram, "position");
        glUniform2fv(positionLoc, 1, glm::value_ptr(entity.position));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glfwSwapBuffers(window);
  }



  // Clean up after ourselves
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();

  return 0;
}
