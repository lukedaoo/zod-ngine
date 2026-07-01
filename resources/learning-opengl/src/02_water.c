#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define COMMON_IMPLEMENTATION
#include "_common.h"

static void check_shader(GLuint sh, const char *name) {
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(sh, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error (%s): %s\n", name, log);
        exit(1);
    }
}

static void check_program(GLuint prog) {
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "Program link error: %s\n", log);
        exit(1);
    }
}

int main(void) {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHintString(GLFW_X11_CLASS_NAME, "learning-opengl");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "learning-opengl");

    GLFWwindow *win = glfwCreateWindow(1920, 1080, "Seascape", NULL, NULL);
    if (!win) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    gladLoadGL(glfwGetProcAddress);

    // clang-format off
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    // clang-format on

    char *v_src = read_file("assets/water.vert");
    char *f_src = read_file("assets/water.glsl");

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, (const char **)&v_src, NULL);
    glCompileShader(vert);
    check_shader(vert, "VS");
    free(v_src);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, (const char **)&f_src, NULL);
    glCompileShader(frag);
    check_shader(frag, "FS");
    free(f_src);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    check_program(prog);
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

    GLint timeLoc  = glGetUniformLocation(prog, "iTime");
    GLint mouseLoc = glGetUniformLocation(prog, "iMouse");
    GLint resLoc   = glGetUniformLocation(prog, "iResolution");

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, 1);

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);

        double mx = 0.0, my = 0.0;
        if (glfwGetWindowAttrib(win, GLFW_HOVERED)) glfwGetCursorPos(win, &mx, &my);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);
        glUniform1f(timeLoc, (float)glfwGetTime());
        glUniform2f(mouseLoc, (float)mx, (float)(h - my));
        glUniform2f(resLoc, (float)w, (float)h);

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
