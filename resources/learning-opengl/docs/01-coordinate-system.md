# OpenGL Coordinate System

## The Screen

OpenGL maps everything into a box from -1 to 1 on each axis.
This box is called **NDC** (Normalized Device Coordinates).

```
         y=1 (top)
           |
x=-1 ------0------ x=1
           |
         y=-1 (bottom)
```

| Point | Coordinate |
|-------|------------|
| Center | `(0, 0)` |
| Top-left | `(-1, 1)` |
| Top-right | `(1, 1)` |
| Bottom-left | `(-1, -1)` |
| Bottom-right | `(1, -1)` |

## Key Rule

**Y goes up** — opposite of screen pixels (where y=0 is top).

```
Screen pixels:   OpenGL NDC:
  0,0 →          -1,1 →
  ↓                ↑
```

## Z axis

Points **toward you** out of the screen. For 2D, always use `z = 0`.

## Example: Triangle at bottom-left

```c
(-1, 0.5)           // up — blue
    |\
    | \
    |  \
(-1,-1) --- (0.5,-1) // right — red
  white
```

Vertices:
```c
-1.0f, -1.0f,  // bottom-left corner  ← this is (-1,-1)
 0.5f, -1.0f,  // right along x-axis
-1.0f,  0.5f,  // up along y-axis
```

## Remember

- Center = `(0,0)`
- Right = x+ 
- Up = y+
- Everything must stay between -1 and 1 or it gets clipped (invisible)
