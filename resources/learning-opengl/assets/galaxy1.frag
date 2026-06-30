// SPDX-License-Identifier: MIT
// Copyright (c) 2026 @YoheiNishitsuji
//[LICENSE] https://opensource.org/licenses/MIT

#version 460 core
uniform vec2 u_resolution;
uniform float u_time;
out vec4 fragColor;

vec3 hsv(float h, float s, float v) {
    vec4 t = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(vec3(h) + t.xyz) * 6.0 - vec3(t.w));
    return v * mix(vec3(t.x), clamp(p - vec3(t.x), 0.0, 1.0), s);
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 r = u_resolution.xy;
    float t = u_time;
    vec4 o = vec4(0.0, 0.0, 0.0, 1.0);
    float i = 0.0, e = 0.0, R = 0.0, s = 0.0;
    vec3 q = vec3(0.0), p,
         d = vec3((fragCoord - 0.5*r)/min(r.y, r.x)*0.5 + vec2(0, 1), 1);

    for (q.yz -= 1.0; i++ < 129.0; ) {
        o.rgb += hsv(-R/i, 0.4, min(R*e*s - 0.07, e)/7.0);
        s = 1.0;
        p = q += d*e*R*0.24;
        p = vec3(log2(R = length(p)) - t*0.5, exp(-p.z/R), atan(p.y, p.x));
        for (e = (p.y -= 1.0); s < 5e2; s += s)
            e += dot(sin(p.yzx*s - t), vec3(0.2) - cos(p.yxy*s))/s*0.2;
    }
    fragColor = o;
}
