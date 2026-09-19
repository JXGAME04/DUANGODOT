// Thu nghiem 3D: dia hinh mesh 4 lop splat (control.rgba = trong so) + lightmap, giong shader "地形" cua bo tham khao.
shader_type spatial;
render_mode unshaded, cull_back, depth_draw_opaque;

uniform sampler2D control_tex : filter_linear_mipmap, repeat_enable;
uniform sampler2D splat0 : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D splat1 : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D splat2 : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D splat3 : source_color, filter_linear_mipmap, repeat_enable;
uniform vec4 st0 = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 st1 = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 st2 = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 st3 = vec4(1.0, 1.0, 0.0, 0.0);
uniform sampler2D lightmap_tex : source_color, filter_linear_mipmap, repeat_disable;
uniform vec4 lm_st = vec4(1.0, 1.0, 0.0, 0.0);
uniform float lm_gain = 1.0;
uniform bool use_lm = true;
uniform int layers = 4;

void fragment() {
	vec4 c = texture(control_tex, UV);
	vec3 col = c.r * texture(splat0, UV * st0.xy + st0.zw).rgb;
	if (layers > 1) { col += c.g * texture(splat1, UV * st1.xy + st1.zw).rgb; }
	if (layers > 2) { col += c.b * texture(splat2, UV * st2.xy + st2.zw).rgb; }
	if (layers > 3) { col += c.a * texture(splat3, UV * st3.xy + st3.zw).rgb; }
	float w = c.r + (layers > 1 ? c.g : 0.0) + (layers > 2 ? c.b : 0.0) + (layers > 3 ? c.a : 0.0);
	if (w > 0.001) { col /= w; }
	if (use_lm) {
		vec2 uv2 = UV2 * lm_st.xy + lm_st.zw;
		uv2.y = 1.0 - uv2.y;
		col *= texture(lightmap_tex, uv2).rgb * lm_gain;
	}
	ALBEDO = col;
}
