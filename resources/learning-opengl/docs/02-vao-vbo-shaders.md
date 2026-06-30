# VAO, VBO & Shaders

## 0. What are these?

**Vertex Shader (`.vert`)**
Runs once per vertex. Takes position data in, outputs where that vertex lands on screen.

**Fragment Shader (`.frag`)**
Runs once per pixel (fragment). Outputs the final color of that pixel.

**VBO — Vertex Buffer Object**
A chunk of memory on the GPU. You upload your vertex data (positions, colors, UVs) here.

**VAO — Vertex Array Object**
Remembers *how* to read the VBO. You configure it once — stride, offsets, which attributes go where — and just bind it at draw time.

```
CPU          →    GPU
float[]      →    VBO (raw bytes)
             →    VAO (how to read those bytes)
             →    Shader (what to do with them)
```

---

## 1. Code — Step by Step

### Step 1: Load and compile shaders

```c
// read file → string → compile on GPU
char *src = read_file("assets/triangle.vert");

GLuint shader = glCreateShader(GL_VERTEX_SHADER);
glShaderSource(shader, 1, (const char **)&src, NULL);
glCompileShader(shader);
free(src);
// repeat for fragment shader
```

### Step 2: Link into a program

```c
// program = vert + frag glued together
GLuint prog = glCreateProgram();
glAttachShader(prog, vert);
glAttachShader(prog, frag);
glLinkProgram(prog);

// shaders compiled into program — no longer needed
glDeleteShader(vert);
glDeleteShader(frag);
```

### Step 3: Create VBO — upload vertex data

```c
// your data on CPU side
float verts[] = {
    //  x      y      r     g     b
     0.0f,  0.5f,  1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
};

GLuint vbo;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);                         // select this VBO
glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts,         // upload to GPU
             GL_STATIC_DRAW);
```

### Step 4: Create VAO — describe memory layout

```c
GLuint vao;
glGenVertexArrays(1, &vao);
glBindVertexArray(vao);         // start recording

// VBO must be bound before describing attributes
glBindBuffer(GL_ARRAY_BUFFER, vbo);

// attribute 0 = position (2 floats, offset 0)
// stride = 5 floats = 20 bytes (x, y, r, g, b)
glVertexAttribPointer(0,                    // attribute index (location=0 in shader)
                      2,                    // 2 components (x, y)
                      GL_FLOAT,             // type
                      GL_FALSE,             // normalize?
                      5 * sizeof(float),    // stride: bytes per vertex
                      NULL);                // offset: starts at byte 0
glEnableVertexAttribArray(0);

// attribute 1 = color (3 floats, offset after x,y = 8 bytes)
glVertexAttribPointer(1,
                      3,                    // 3 components (r, g, b)
                      GL_FLOAT,
                      GL_FALSE,
                      5 * sizeof(float),    // same stride
                      (void *)(2 * sizeof(float)));  // skip x,y = 8 bytes
glEnableVertexAttribArray(1);

glBindVertexArray(0);           // stop recording
```

Memory layout of one vertex:
```
byte: 0    4    8    12   16   20
      [ x ][ y ][ r ][ g ][ b ]
       attr0    attr1
       ←2f→     ←3f →
      ←────── stride 5f ──────→
```

### Step 5: Shader files — in/out

`assets/triangle.vert`:
```glsl
#version 460 core

layout(location=0) in vec2 pos;    // matches attribute index 0
layout(location=1) in vec3 color;  // matches attribute index 1

out vec3 v_color;                  // pass to fragment shader

void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    v_color = color;
}
```

`assets/triangle.frag`:
```glsl
#version 460 core

in vec3 v_color;       // received from vertex shader
out vec4 frag_color;   // final pixel color output

void main() {
    frag_color = vec4(v_color, 1.0);  // rgb + alpha=1
}
```

### Step 6: Draw loop — binding order

```c
// every frame:
glUseProgram(prog);      // 1. activate shader program
glBindVertexArray(vao);  // 2. bind VAO (it knows the VBO + layout)
glDrawArrays(GL_TRIANGLES, 0, 3);  // 3. draw 3 vertices
```

---

## Summary

```
1. Compile vert + frag shaders
2. Link into program
3. Upload vertex data → VBO
4. Describe layout → VAO
5. Draw: useProgram → bindVAO → drawArrays
```

VAO is the key — bind once, reuse every frame. No need to re-describe the layout each draw.
