// Ban mo (CameraBuildingFade cua ban tham khao: nha che camera mo xuong FadeAlpha 0,25 voi FadeSpeed 10/s): nhu scn3d_lm nhung ghi ALPHA.
// Thu nghiem 3D: vat the tinh co lightmap. Lightmap cua bo tham khao chi chua anh sang gian tiep/bau troi
// (Unity Mixed - Baked Indirect), nang mat troi la den thoi gian thuc -> o day: den DirectionalLight3D + bong
// thoi gian thuc chieu len ALBEDO, lightmap cong them qua EMISSION (albedo * lm * gain), tat ambient cua moi truong.
shader_type spatial;
render_mode cull_back, depth_draw_always, ambient_light_disabled, specular_disabled;

uniform sampler2D albedo_tex : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D lightmap_tex : filter_linear_mipmap, repeat_disable;
uniform vec4 lm_st = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 tint : source_color = vec4(1.0);
uniform float lm_gain = 1.0;
uniform bool use_lm = true;
uniform bool alpha_test = false;
uniform float cutoff = 0.5;
uniform float fade = 1.0;

void fragment() {
	vec4 a = texture(albedo_tex, UV) * tint;
	if (alpha_test && a.a < cutoff) {
		discard;
	}
	vec2 uv2 = UV2 * lm_st.xy + lm_st.zw;
	vec3 lm = use_lm ? texture(lightmap_tex, uv2).rgb * lm_gain : vec3(0.35);
	ALBEDO = a.rgb;
	EMISSION = a.rgb * lm;
	ALPHA = a.a * fade;
	ROUGHNESS = 1.0;
	METALLIC = 0.0;
}
