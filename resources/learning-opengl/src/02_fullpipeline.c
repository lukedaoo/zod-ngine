#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

static const GLenum MODES[]     = {GL_FILL, GL_LINE, GL_POINT};
static const char  *MODE_NAMES[] = {"GL_FILL", "GL_LINE", "GL_POINT"};
static int          mode_idx    = 0;

static void key_cb(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, 1);
    if (key == GLFW_KEY_SPACE) {
        mode_idx = (mode_idx + 1) % 3;
        glPolygonMode(GL_FRONT_AND_BACK, MODES[mode_idx]);
        printf("polygon mode: %s\n", MODE_NAMES[mode_idx]);
    }
}

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
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(800, 600, "Tessellation Pipeline", NULL, NULL);
    if (!win) return 1;

    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, key_cb);
    gladLoadGL(glfwGetProcAddress);
    glPointSize(4.0f);

    const char *vs_src =
         "#version 460 core\n"
         "layout(location = 0) in vec2 aPos;\n"
         "void main() {\n"
         "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
         "}\n";

    const char *tcs_src =
         "#version 460 core\n"
         "layout(vertices = 3) out;\n"
         "void main() {\n"
         "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
         "    gl_TessLevelOuter[0] = 8.0;\n"
         "    gl_TessLevelOuter[1] = 8.0;\n"
         "    gl_TessLevelOuter[2] = 8.0;\n"
         "    gl_TessLevelInner[0] = 8.0;\n"
         "}\n";

    const char *tes_src =
         "#version 460 core\n"
         "layout(triangles, equal_spacing, cw) in;\n"
         "void main() {\n"
         "    vec3 p0 = gl_in[0].gl_Position.xyz;\n"
         "    vec3 p1 = gl_in[1].gl_Position.xyz;\n"
         "    vec3 p2 = gl_in[2].gl_Position.xyz;\n"
         "    vec3 pos = p0 * gl_TessCoord.x + p1 * gl_TessCoord.y + p2 * "
         "gl_TessCoord.z;\n"
         "    gl_Position = vec4(pos, 1.0);\n"
         "}\n";

    const char *fs_src =
         "#version 460 core\n"
         "out vec4 fragColor;\n"
         "void main() {\n"
         "    float r = 0.3 + 0.7 * fract(sin(float(gl_PrimitiveID) * 127.1) * 43758.5);\n"
         "    float g = 0.3 + 0.7 * fract(sin(float(gl_PrimitiveID) * 269.5) * 12345.6);\n"
         "    float b = 0.3 + 0.7 * fract(sin(float(gl_PrimitiveID) * 419.2) * 98765.4);\n"
         "    fragColor = vec4(r, g, b, 1.0);\n"
         "}\n";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_src, NULL);
    glCompileShader(vs);
    check_shader(vs, "VS");

    GLuint tcs = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tcs, 1, &tcs_src, NULL);
    glCompileShader(tcs);
    check_shader(tcs, "TCS");

    GLuint tes = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tes, 1, &tes_src, NULL);
    glCompileShader(tes);
    check_shader(tes, "TES");

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_src, NULL);
    glCompileShader(fs);
    check_shader(fs, "FS");

    // --- link program ---
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, tcs);
    glAttachShader(prog, tes);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    check_program(prog);

    glDeleteShader(vs);
    glDeleteShader(tcs);
    glDeleteShader(tes);
    glDeleteShader(fs);

    // clang-format off
    float vertices[] = {
         -0.5f, -0.5f, 
         0.5f, -0.5f, 
         0.0f, 0.5f,
    };
    // clang-format on

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glPatchParameteri(GL_PATCH_VERTICES, 3);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glBindVertexArray(VAO);

        glDrawArrays(GL_PATCHES, 0, 3);

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(prog);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
