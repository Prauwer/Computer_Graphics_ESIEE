attribute vec3 a_position;
attribute vec3 a_color;
attribute vec2 a_uv;
varying vec3 v_color;
varying vec2 v_uv;


uniform mat4 uScale;
uniform mat4 uRotation;
uniform mat4 uTranslation;

void main(void) {
 gl_Position = uTranslation * uRotation * uScale * vec4(a_position, 1.0);

 v_color = a_color;
 v_uv = vec2(a_uv.s, 1.0 - a_uv.t);

}
