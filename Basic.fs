precision mediump float;
varying vec3 v_color;
varying vec2 v_uv;
uniform sampler2D uTexture;  // dragon.png

void main(void) {
    vec4 texColor = texture2D(uTexture, v_uv);
    gl_FragColor = vec4(v_color, 1.0) * texColor;
}