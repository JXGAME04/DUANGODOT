// La cay / co dung dua: nhu scn3d_lm (lightmap gian tiep + nang thoi gian thuc, alpha test) them dich dinh theo gio.
// Tham so lay tu vat lieu cua ban tham khao (scene.json floats): la "Alpha剪裁_树叶_自圆周晃động" (_radius, speed, noroff),
// co "Alpha剪裁_草_晃动_顶点Alpha影响" (windSpeed, windStr, gHeight, speed). Shader goc da bien dich nen CONG THUC dich
// dinh o day la tu viet [tự chọn], chi cac con so la cua ban tham khao.
shader_type spatial;
render_mode cull_disabled, depth_draw_opaque, ambient_light_disabled, specular_disabled;

uniform sampler2D albedo_tex : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D lightmap_tex : filter_linear_mipmap, repeat_disable;
uniform vec4 lm_st = vec4(1.0, 1.0, 0.0, 0.0);
uniform vec4 tint : source_color = vec4(1.0);
uniform float lm_gain = 1.0;
uniform bool use_lm = true;
uniform bool alpha_test = true;
uniform float cutoff = 0.5;
uniform int sway_mode = 1;        // 1 la cay (quay tron quanh goc), 2 co (gio mot chieu, theo alpha dinh)
uniform float sway_speed = 3.0;   // speed
uniform float sway_amount = 0.2;  // noroff (la) / windStr (co), met
uniform float sway_radius = 10.0; // _radius (la): cao do chuan hoa; gHeight (co)
uniform float wind_speed = 1.5;   // windSpeed (co)

void vertex() {
	vec3 wp = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	float phase = wp.x * 0.35 + wp.z * 0.23;
	float h = clamp(VERTEX.y / max(sway_radius, 0.01), 0.0, 1.0);
	if (sway_mode == 1) {
		// tan la quay tron nhe, cang cao cang rong
		float t = TIME * sway_speed + phase;
		VERTEX.x += sin(t) * sway_amount * h;
		VERTEX.z += cos(t * 0.83) * sway_amount * h;
	} else {
		// ngon co nga theo gio, dinh (alpha dinh cao) nga nhieu hon
		float w = h * (COLOR.a > 0.0 ? COLOR.a : 1.0);
		float t = TIME * wind_speed + phase;
		VERTEX.x += (sin(t) * 0.7 + sin(t * 2.3 + 1.0) * 0.3) * sway_amount * w;
		VERTEX.z += cos(t * 0.6) * sway_amount * 0.4 * w;
	}
}

void fragment() {
	vec4 a = texture(albedo_tex, UV) * tint;
	if (alpha_test && a.a < cutoff) {
		discard;
	}
	vec2 uv2 = UV2 * lm_st.xy + lm_st.zw;
	vec3 lm = use_lm ? texture(lightmap_tex, uv2).rgb * lm_gain : vec3(0.35);
	ALBEDO = a.rgb;
	EMISSION = a.rgb * lm;
	ROUGHNESS = 1.0;
	METALLIC = 0.0;
}
