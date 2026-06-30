#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define COMMON_IMPLEMENTATION
#include "_common.h"

static GLuint compile_shader(GLenum type, const char *path) {
    char *src = read_file(path);
    if (!src) return 0;
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, (const char **)&src, NULL);
    glCompileShader(s);
    free(src);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        fprintf(stderr, "shader error (%s): %s\n", path, buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

// clang-format off
static const float VERTS[] = {
    //x       y      r     g     b
    0.0f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
};
// clang-format on

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "learning-opengl");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "learning-opengl");

    GLFWwindow *win = glfwCreateWindow(400, 300, "01 triangle", NULL, NULL);
    glfwMakeContextCurrent(win);
    gladLoadGL(glfwGetProcAddress);

    GLuint vert = compile_shader(GL_VERTEX_SHADER, "assets/triangle.vert");
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, "assets/triangle.frag");
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

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, 1);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
