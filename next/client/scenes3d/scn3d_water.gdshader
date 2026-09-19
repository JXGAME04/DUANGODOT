// Nuoc: vat lieu "水体高光双浪_水底扭曲" (hai lop song phap tuyen, day nuoc bi vặn) cua ban tham khao. Cac con so
// (_NormalTex tiling, _FlowDirX/Y, _FlowSpeed, _ripple, _DistortStrength, _DistortTimeFactor, _Brightness, _SpecSize,
// _SpecSt, _Alpha, _AlphaAdd, _BlendRefBottom, mau) doc tu scene.json; shader goc da bien dich nen cong thuc la tu viet
// [tự chọn]: hai lop phap tuyen chay nguoc chieu, day nuoc (BottomTex) lay theo UV bi vặn, phan xa (ReflectTex) pha
// theo goc nhin, diem sang mat troi theo phap tuyen.
shader_type spatial;
render_mode cull_disabled, depth_draw_never, blend_mix, ambient_light_disabled, specular_disabled;

uniform sampler2D normal_tex : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D bottom_tex : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D reflect_tex : source_color, filter_linear_mipmap, repeat_enable;
uniform vec4 normal_st = vec4(12.0, 6.0, 0.0, 0.0);
uniform vec4 water_color : source_color = vec4(0.137, 0.446, 0.576, 1.0);
uniform vec2 flow_dir = vec2(0.597, -0.012);
uniform float flow_speed = 0.028;
uniform float ripple = 4.05;
uniform float distort = 0.063;
uniform float distort_time = 1.669;
uniform float brightness = 3.27;
uniform float spec_size = 1000.0;
uniform float spec_strength = 0.64;
uniform float alpha = 1.0;
uniform float alpha_add = 0.898;
uniform float blend_ref_bottom = 0.153;
uniform vec3 sun_dir = vec3(-0.3, -0.85, 0.4);

void fragment() {
	vec2 uv = UV * normal_st.xy + normal_st.zw;
	vec2 f = flow_dir * flow_speed * TIME;
	// hai lop song: mot theo dong, mot nguoc va cham hon (双浪)
	vec3 n1 = texture(normal_tex, uv + f * ripple).xyz * 2.0 - 1.0;
	vec3 n2 = texture(normal_tex, uv * 0.7 - f * ripple * 0.6 + vec2(0.3)).xyz * 2.0 - 1.0;
	vec3 n = normalize(vec3(n1.xy + n2.xy, n1.z * n2.z));
	// day nuoc bi vặn theo thoi gian
	vec2 duv = UV + n.xy * distort * (0.6 + 0.4 * sin(TIME * distort_time));
	vec3 bottom = texture(bottom_tex, duv).rgb;
	vec3 refl = texture(reflect_tex, UV * 2.0 + n.xy * 0.05).rgb;
	// goc nhin: nhin xien thay phan xa, nhin thang thay day
	vec3 view = normalize(-VERTEX);
	vec3 nv = normalize(NORMAL + vec3(n.x, 0.0, n.y) * 0.3);
	float fres = pow(1.0 - clamp(dot(view, nv), 0.0, 1.0), 3.0);
	float refl_w = clamp(blend_ref_bottom + fres * (1.0 - blend_ref_bottom), 0.0, 1.0);
	vec3 col = mix(bottom * water_color.rgb, refl, refl_w) * brightness * 0.35;
	// diem sang mat troi
	vec3 l = normalize(-(VIEW_MATRIX * vec4(sun_dir, 0.0)).xyz);
	vec3 hv = normalize(l + view);
	float spec = pow(clamp(dot(nv, hv), 0.0, 1.0), spec_size * 0.05) * spec_strength;
	ALBEDO = col;
	EMISSION = col * 0.5 + vec3(spec);
	ALPHA = clamp(alpha * (alpha_add + fres * 0.3), 0.0, 1.0);
	ROUGHNESS = 0.1;
	METALLIC = 0.0;
}
