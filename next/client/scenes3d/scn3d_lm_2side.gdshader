// Thu nghiem 3D: vat the tinh da nuong lightmap (albedo * lightmap(UV2 * st.xy + st.zw)).
// Lightmap (BC6H HDR, tuyen tinh, PNG khong lat) -> sRGB (mu 1/2.2) truoc khi nhan voi albedo sRGB.
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque;

uniform sampler2D albedo_tex : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D lightmap_tex : filter_linear_mipmap, repeat_disable;
uniform vec4 lm_st = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 tint : source_color = vec4(1.0);
uniform float lm_gain = 1.0;
uniform bool use_lm = true;
uniform bool alpha_test = false;
uniform float cutoff = 0.5;

void fragment() {
	vec4 a = texture(albedo_tex, UV) * tint;
	if (alpha_test && a.a < cutoff) {
		discard;
	}
	vec2 uv2 = UV2 * lm_st.xy + lm_st.zw;
	vec3 lm = use_lm ? pow(max(texture(lightmap_tex, uv2).rgb, vec3(0.0)), vec3(1.0 / 2.2)) * lm_gain : vec3(1.0);
	ALBEDO = a.rgb * lm;
}
