#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define COMMON_IMPLEMENTATION
#include "_common.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "learning-opengl");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "learning-opengl");

    GLFWwindow *win = glfwCreateWindow(400, 300, "02 draw a line", NULL, NULL);
    glfwMakeContextCurrent(win);
    gladLoadGL(glfwGetProcAddress);

    const char *vert =
         "#version 460 core\n"
         "layout(location=0) in vec2 pos;\n"
         "layout(location=1) in vec3 color;\n"
         "out vec3 v_color;\n"
         "void main() {\n"
         "    gl_Position = vec4(pos, 0.0, 1.0);\n"
         "    v_color = color;\n"
         "}\n";

    const char *frag =
         "#version 460 core\n"
         "in vec3 v_color;\n"
         "out vec4 frag_color;\n"
         "void main() {\n"
         "    frag_color = vec4(v_color, 1.0);\n"
         "}\n";

    // ```
    //          y=1 (top)
    //            |
    // x=-1 ------0------ x=1
    //            |
    //          y=-1 (bottom)
    // ```
    // clang-format off
    float vertices[] = {
        -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    };
    // clang-format on

    // Compile shader

    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert, NULL);
    glCompileShader(vert_shader);

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag, NULL);
    glCompileShader(frag_shader);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vert_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, 1);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, 2);

        glfwSwapBuffers(win);
    }

    return 0;
}
