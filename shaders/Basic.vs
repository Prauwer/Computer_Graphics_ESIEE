attribute vec3 a_position;
attribute vec3 a_color;
attribute vec2 a_uv;
varying vec3 v_color;
varying vec2 v_uv;


uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uModelMatrix;

void main(void) {
 gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(a_position, 1.0);

 v_color = a_color;
 v_uv = vec2(a_uv.s, 1.0 - a_uv.t);

}
