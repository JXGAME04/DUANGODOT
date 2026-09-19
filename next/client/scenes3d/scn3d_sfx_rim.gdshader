// Rim-lit effect mesh (the dragon of 飞龙在天): the reference's particle/blend_dst_zw_ver_rimlight [TK GLSL, shaders_apk]:
//   base  = tex.rgb x (1 + max(uv.z, 0)) x _Enhance            (uv.z = 0 for a plain mesh)
//   alpha = min(tex.a x _AdjustA x clamp(uv.z + 1, 0, 1), 1)    discard under 0.02
//   rim   = pow(1 - max(dot(N, V), 0), _RimPower)
//   edge  = vcol.rgb x _EdgeColor.rgb x (vcol.a x 25 + 1)        vcol = SFXMeshModify colour + emissive when the node has it
//   rgb   = mix(base, edge, rim); Blend SrcAlpha OneMinusSrcAlpha, ZWrite on
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque, blend_mix;

uniform sampler2D tex: source_color, filter_linear_mipmap;
uniform vec4 edge_color: source_color = vec4(1.0);
uniform float rim_power = 2.0;
uniform float adjust_a = 1.0;
uniform float enhance = 1.0;
uniform vec4 vcol_mod: source_color = vec4(1.0);
uniform bool use_vcol_mod = false;
uniform vec2 uv_scale = vec2(1.0);
uniform vec2 uv_offset = vec2(0.0);

varying vec3 v_normal_w;
varying vec3 v_view_w;
varying vec4 v_col;

void vertex() {
	v_normal_w = normalize(MODEL_NORMAL_MATRIX * NORMAL);
	v_view_w = CAMERA_POSITION_WORLD - (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	v_col = COLOR;
}

void fragment() {
	vec4 t = texture(tex, UV * uv_scale + uv_offset);
	float a = min(t.a * adjust_a, 1.0);
	if (a < 0.02) {
		discard;
	}
	vec3 base = t.rgb * enhance;
	float rim = pow(1.0 - max(dot(normalize(v_normal_w), normalize(v_view_w)), 0.0), rim_power);
	vec4 vc = use_vcol_mod ? vcol_mod : v_col;
	vec3 edge = vc.rgb * edge_color.rgb * (vc.a * 25.0 + 1.0);
	ALBEDO = clamp(mix(base, edge, rim), 0.0, 1.0);
	ALPHA = a;
}
