// SFXMixerMesh of the reference client [TK]: a stack of atlas-cell quads in the XZ plane (SFXMixerMesh..cctor: the
// template quad is (+-0.5, 0, +-0.5); InitMeshData: layers from the last to the first, each 0.01 m higher;
// FillMeshSquare: uv = cell of TexTrans (nx, ny, ox, oy), v counted from the top) animated by the vertex shader
// 剑网江湖/particle/sfx_mixer_mesh_rs (GLSL of the APK build, D:\game3gtQ_mo\shaders_apk):
//   uv0.zw = (rotation rad/s, or a fixed angle +-1000 when a curve drives it on the CPU; enhance)
//   uv1    = (scale type (+ a period tweak in its hundredths), scale speed, scale pos, scale dis)
//   NORMAL = the radial direction the vertex scales along, TANGENT.zx = the layer centre the quad turns around.
// Here: CUSTOM0 = (rot, enhance, scale type, scale speed), CUSTOM1 = (scale pos, scale dis, centre x, centre z).
// The exporter mirrors X (Unity -> Godot), so the turn goes the other way (-angle) and the centre x is mirrored.
uniform sampler2D tex : source_color, filter_linear_mipmap;
uniform vec4 tint : source_color = vec4(1.0);

void vertex() {
	vec4 c0 = CUSTOM0;
	vec4 c1 = CUSTOM1;
	vec3 n = normalize(NORMAL);
	float t = c0.w * TIME;
	vec3 p = VERTEX;
	float st = round(c0.z);
	float fade = 1.0;
	if (st == 1.0) {
		// pulsing: pos .. pos + |dis| on a sine
		p += n * ((sin(t) * 0.5 + 0.5) * abs(c1.y) + c1.x);
	} else if (st == 2.0) {
		// waves: a sawtooth grows (dis > 0) or shrinks (dis < 0) the quad, fading out over its last fifth
		float per = 1.0 - fract(abs(c0.z * 0.01));
		float saw = fract(t * per) / per;
		float s = c1.y < 0.0 ? 1.0 : 0.0;
		p += n * (c1.x + abs(s - saw) * abs(c1.y));
		fade = 1.0 - clamp((saw - 0.8) * 5.0, 0.0, 1.0);
	}
	float ang = c0.x * TIME;
	if (c0.x < -1000.0) {
		ang = c0.x + 1000.0;
	} else if (c0.x > 1000.0) {
		ang = c0.x - 1000.0;
	}
	ang = -ang;
	vec2 d = vec2(p.z - c1.w, p.x - c1.z);
	float sn = sin(ang);
	float cs = cos(ang);
	vec2 r = vec2(d.x * cs + d.y * sn, d.y * cs - d.x * sn);
	p.z = r.x + c1.w;
	p.x = r.y + c1.z;
	VERTEX = p;
	COLOR = COLOR * c0.y;
	COLOR.a *= fade;
}

void fragment() {
	vec4 c = texture(tex, UV) * tint * COLOR;
	ALBEDO = c.rgb;
	ALPHA = min(c.a, 1.0);
}
