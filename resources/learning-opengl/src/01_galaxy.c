#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define COMMON_IMPLEMENTATION
#include "_common.h"

int main(void) {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHintString(GLFW_X11_CLASS_NAME, "learning-opengl");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "learning-opengl");

    GLFWwindow *win = glfwCreateWindow(800, 600, "Shader Display", NULL, NULL);
    if (!win) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    gladLoadGL(glfwGetProcAddress);

    float vertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
                        1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f};

    char *v_src = read_file("assets/galaxy1.vert");
    char *f_src = read_file("assets/galaxy1.frag");

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, (const char **)&v_src, NULL);
    glCompileShader(vert);
    free(v_src);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, (const char **)&f_src, NULL);
    glCompileShader(frag);
    free(f_src);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    GLint resLoc  = glGetUniformLocation(prog, "u_resolution");
    GLint timeLoc = glGetUniformLocation(prog, "u_time");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glUniform2f(resLoc, (float)w, (float)h);
        glUniform1f(timeLoc, (float)glfwGetTime());

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(prog);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
