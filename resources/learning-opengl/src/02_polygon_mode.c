#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#define COMMON_IMPLEMENTATION
#include "_common.h"

static const char *VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "layout(location=1) in vec3 color;\n"
     "out vec3 v_color;\n"
     "void main() {\n"
     "    gl_Position = vec4(pos, 0.0, 1.0);\n"
     "    v_color = color;\n"
     "}\n";

static const char *FRAG_SRC =
     "#version 460 core\n"
     "in vec3 v_color;\n"
     "out vec4 frag_color;\n"
     "void main() { frag_color = vec4(v_color, 1.0); }\n";

// clang-format off
static const float VERTS[] = {
    // x      y      r     g     b

    // triangle (top-left)
    -0.8f,  0.8f,  1.0f, 0.2f, 0.2f,
    -0.2f,  0.8f,  1.0f, 0.2f, 0.2f,
    -0.5f,  0.2f,  1.0f, 0.2f, 0.2f,

    // triangle (top-right)
     0.2f,  0.8f,  0.2f, 1.0f, 0.2f,
     0.8f,  0.8f,  0.2f, 1.0f, 0.2f,
     0.5f,  0.2f,  0.2f, 1.0f, 0.2f,

    // quad = 2 triangles (bottom-left)
    -0.8f, -0.2f,  0.2f, 0.4f, 1.0f,
    -0.2f, -0.2f,  0.2f, 0.4f, 1.0f,
    -0.2f, -0.8f,  0.2f, 0.4f, 1.0f,
    -0.8f, -0.2f,  0.2f, 0.4f, 1.0f,
    -0.2f, -0.8f,  0.2f, 0.4f, 1.0f,
    -0.8f, -0.8f,  0.2f, 0.4f, 1.0f,

    // pentagon (bottom-right, 3 tris fan)
     0.5f, -0.2f,  1.0f, 0.8f, 0.0f,
     0.8f, -0.4f,  1.0f, 0.8f, 0.0f,
     0.6f, -0.8f,  1.0f, 0.8f, 0.0f,
     0.5f, -0.2f,  1.0f, 0.8f, 0.0f,
     0.6f, -0.8f,  1.0f, 0.8f, 0.0f,
     0.2f, -0.8f,  1.0f, 0.8f, 0.0f,
     0.5f, -0.2f,  1.0f, 0.8f, 0.0f,
     0.2f, -0.8f,  1.0f, 0.8f, 0.0f,
     0.2f, -0.4f,  1.0f, 0.8f, 0.0f,
};
// clang-format on

static const char  *MODE_NAMES[] = {"GL_FILL", "GL_LINE", "GL_POINT"};
static const GLenum MODES[]      = {GL_FILL, GL_LINE, GL_POINT};
static int          mode_idx     = 0;

static void key_cb(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, 1);
    if (key == GLFW_KEY_SPACE) {
        mode_idx = (mode_idx + 1) % 3;
        printf("polygon mode: %s\n", MODE_NAMES[mode_idx]);
        glPolygonMode(GL_FRONT_AND_BACK, MODES[mode_idx]);
    }
}

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "learning-opengl");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "learning-opengl");

    GLFWwindow *win =
         glfwCreateWindow(600, 600, "03 polygon mode  [SPACE=cycle]", NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_cb);
    gladLoadGL(glfwGetProcAddress);

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &VERT_SRC, NULL);
    glCompileShader(vert);
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &FRAG_SRC, NULL);
    glCompileShader(frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTS), VERTS, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), NULL);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(2 * sizeof(float)));
    glBindVertexArray(0);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glPointSize(4.0f);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(VERTS) / (5 * sizeof(float)));
        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
