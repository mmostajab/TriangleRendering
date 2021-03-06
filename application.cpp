#ifdef WIN32
#include <Windows.h>
#endif

#include "application.h"

// STD
#include <iostream>
#include <fstream>
#include <time.h>
#include <random>

// GL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>


// Static Members
GLFWwindow*				    Application::m_window = 0;
unsigned int			    Application::m_width = 0;
unsigned int			    Application::m_height = 0;
bool					        Application::m_controlKeyHold = false;
bool					        Application::m_altKeyHold = false;
bool					        Application::m_w_pressed = false;
bool					        Application::m_s_pressed = false;
bool					        Application::m_a_pressed = false;
bool					        Application::m_d_pressed = false;
bool					        Application::m_q_pressed = false;
bool					        Application::m_e_pressed = false;
bool                  Application::m_mouse_left_drag = false;
bool                  Application::m_mouse_middle_drag = false;
bool                  Application::m_mouse_right_drag = false;

// read the file content and generate a string from it.
static std::string convertFileToString(const std::string& filename) {
  std::ifstream ifile(filename);
  if (!ifile){
    return std::string("");
  }

  return std::string(std::istreambuf_iterator<char>(ifile), (std::istreambuf_iterator<char>()));

}

static void show_compiler_error(GLuint shader) {
  GLint isCompiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE)
  {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

    std::cout << "Shader " << shader << " Log: \n";
    std::cout << ((char*)&errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(shader); // Don't leak the shader.
    return;
  }
}

static GLuint compile_link_vs_fs(const std::string& vert_shader_file, const std::string& frag_shader_file) {
  GLuint vertex_shader, fragment_shader;
  std::string vertex_shader_source = convertFileToString(vert_shader_file);
  std::string coord_sys_shader_fragment_source = convertFileToString(frag_shader_file);

  if (vertex_shader_source.size() == 0 || coord_sys_shader_fragment_source.size() == 0){
    std::cout << "Problem in finding " << vert_shader_file << " or " << frag_shader_file << std::endl;
    return -1;
  }


  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  const GLchar* vertex_shader_sourcePtr = vertex_shader_source.c_str();
  const GLchar* fragment_shader_sourcePtr = coord_sys_shader_fragment_source.c_str();

  glShaderSource(vertex_shader, 1, &vertex_shader_sourcePtr, NULL);
  glShaderSource(fragment_shader, 1, &fragment_shader_sourcePtr, NULL);

  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  show_compiler_error(vertex_shader);
  show_compiler_error(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgramARB(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return shader_program;
}


Application::Application() {
  initialization_step = true;
  m_worldmat = m_viewmat = m_projmat = glm::mat4(1.0f);
}

void Application::init(const unsigned int& width, const unsigned int& height) {

  m_width = width; m_height = height;

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  m_window = glfwCreateWindow(width, height, "Stream Surface Generator (Demo): Beta", NULL, NULL);
  if (!m_window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(m_window);

  glfwSetKeyCallback(m_window, key_callback);
  glfwSetWindowSizeCallback(m_window, WindowSizeCB);
  glfwSetCursorPosCallback(m_window, EventMousePos);
  glfwSetScrollCallback(m_window, EventMouseWheel);
  glfwSetMouseButtonCallback(m_window, (GLFWmousebuttonfun)EventMouseButton);
  glfwSetKeyCallback(m_window, (GLFWkeyfun)EventKey);

  // - Directly redirect GLFW char events to AntTweakBar
  glfwSetCharCallback(m_window, (GLFWcharfun)EventChar);

  if (glewInit() != GLEW_OK){
    std::cout << "cannot intialize the glew.\n";
    exit(EXIT_FAILURE);
  }

  init();
}

void Application::init() {
  GLenum e = glGetError();
  glClearColor(19.0f / 255.0f, 9.0f / 255.0f, 99.0f / 255.0f, 1.0f);
  glClearDepth(1.0f);
  e = glGetError();

  e = glGetError();
  glEnable(GL_DEPTH_TEST);
  e = glGetError();
}

void Application::create() {
  compileShaders();

  float vertexPositions[] =
  {
    0.75f, 0.75f, 0.0f, 1.0f,
    0.75f, -0.75f, 0.0f, 1.0f,
    -0.75f, -0.75f, 0.0f, 1.0f,
  };

  glGenBuffers(1, &positionBufferObject);
  glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
}

void Application::update(float time, float timeSinceLastFrame) {
  float v = (float)clock() / 3000.0f * glm::pi<float>();
  m_inv_viewmat = glm::inverse(m_viewmat);
  m_viewmat = glm::lookAt(glm::vec3(sin(v), 1.0f, cos(v)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  m_projmat = glm::perspective(glm::pi<float>() / 3.0f, (float)m_width / m_height, 0.1f, 1000.0f);
}

void Application::draw() {
  GLenum e = glGetError();
  glViewport(0, 0, m_width, m_height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glUseProgram(shader);

  glEnableVertexAttribArray(0);

  // tell openGL how to interpret the array of data stored in buffer
  // this implicitly refers to the buffer currently bound to GL_ARRAY_BUFFER
  glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // rendering function
  glDrawArrays(GL_TRIANGLES, 0, 3);

  e = glGetError();

  glDisableVertexAttribArray(0);
}

void Application::run() {
  create();
  double start_time;
  double start_frame;
  start_time = start_frame = glfwGetTime();

  while (!glfwWindowShouldClose(m_window))
  {
    double frame_start_time = glfwGetTime();
    draw();
    double frame_end_time = glfwGetTime();


    glfwSwapBuffers(m_window);
    glfwPollEvents();

    double current_time = glfwGetTime();
    double elapsed_since_start = current_time - start_time;
    double elapsed_since_last_frame = current_time - start_frame;

    start_frame = glfwGetTime();

    update(elapsed_since_start, elapsed_since_last_frame);
  }
}

void Application::shutdown() {
  glfwDestroyWindow(m_window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

Application::~Application() {
}

void Application::compileShaders() {
  shader = compile_link_vs_fs("vertex.glsl", "fragment.glsl");
}

void Application::EventMouseButton(GLFWwindow* window, int button, int action, int mods) {
 
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    m_mouse_left_drag = true;

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    m_mouse_left_drag = false;

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    m_mouse_right_drag = true;

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    m_mouse_right_drag = false;

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    m_mouse_middle_drag = true;

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    m_mouse_middle_drag = false;
}

void Application::EventMousePos(GLFWwindow* window, double xpos, double ypos) {
}

void Application::EventMouseWheel(GLFWwindow* window, double xoffset, double yoffset) {

}

void Application::EventKey(GLFWwindow* window, int key, int scancode, int action, int mods) {

  if (action == GLFW_PRESS){
    if (m_controlKeyHold && key == GLFW_KEY_W)  m_w_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_S)  m_s_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_A)  m_a_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_D)  m_d_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_Q)  m_q_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_E)  m_e_pressed = true;

    if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = true;
    if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold = true;
  }

  if (action == GLFW_RELEASE){
    if (key == GLFW_KEY_W)  m_w_pressed = false;
    if (key == GLFW_KEY_S)  m_s_pressed = false;
    if (key == GLFW_KEY_A)  m_a_pressed = false;
    if (key == GLFW_KEY_D)  m_d_pressed = false;
    if (key == GLFW_KEY_Q)  m_q_pressed = false;
    if (key == GLFW_KEY_E)  m_e_pressed = false;

    if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = false;
    if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold = false;

    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
  }
}

void Application::EventChar(GLFWwindow* window, int codepoint) {
}

// Callback function called by GLFW when window size changes
void Application::WindowSizeCB(GLFWwindow* window, int width, int height) {
  m_width = width; m_height = height;
  glViewport(0, 0, width, height);
}
void Application::error_callback(int error, const char* description) {
  fputs(description, stderr);
}
void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
  }

  if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS){

  }

  if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE){
  }
}
