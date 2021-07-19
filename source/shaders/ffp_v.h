const char *ffp_vert_src =
R"(#define clip_planes_num %d
#define num_textures %d
#define has_colors %d
#define lights_num %d
#define shading_mode %d

#if lights_num > 0 && shading_mode == 1 // GL_SMOOTH
static float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

uniform float4 lights_ambients[lights_num];
uniform float4 lights_diffuses[lights_num];
uniform float4 lights_speculars[lights_num];
uniform float4 lights_positions[lights_num];
uniform float3 lights_attenuations[lights_num];

void point_light(int i, float3 normal, float3 eye, float3 position) {
	float3 VP = lights_positions[i].xyz - position;
	float d = length(VP);
	VP = normalize(VP);
	float attenuation = 1.0f / (lights_attenuations[i].x +
		lights_attenuations[i].y * d +
		lights_attenuations[i].z * d * d);
	float3 halfVector = normalize(VP + eye);
	float nDotVP = max(0.0f, dot(normal, VP));
	float nDotHV = max(0.0f, dot(normal, halfVector));
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
	Ambient += lights_ambients[i] * attenuation;
	Diffuse += lights_diffuses[i] * nDotVP * attenuation;
	Specular += lights_speculars[i] * pf * attenuation;
}

void directional_light(int i, float3 normal) {
	float nDotVP = max(0.0f, dot(normal, normalize(lights_positions[i].xyz)));
	float nDotHV = max(0.0f, dot(normal, normalize(normalize(lights_positions[i].xyz) + float3(0.0f, 0.0f, 1.0f))));
	
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
		
	Ambient += lights_ambients[i];
	Diffuse += lights_diffuses[i] * nDotVP;
	Specular += lights_speculars[i] * pf;
}

void calculate_light(int i, float3 ecPosition, float3 N) {
	float3 eye = float3(0.0f, 0.0f, 1.0f);
	
	if (lights_positions[i].w == 1.0f)
		point_light(i, N, eye, ecPosition);
	else
		directional_light(i, N);
}
#endif

void main(
	float3 position,
#if num_textures > 0
	float2 texcoord0,
#if num_textures > 1
	float2 texcoord1,
#endif
#endif
#if has_colors == 1
	float4 color, // We re-use this for ambient values when lighting is on
#endif
#if lights_num > 0
	float4 diff,
	float4 spec,
	float4 emission,
	float3 normals,
#endif
#if num_textures > 0
	float2 out vTexcoord : TEXCOORD0,
#if num_textures > 1
	float2 out vTexcoord2 : TEXCOORD1,
#endif
#endif
#if lights_num > 0 && shading_mode == 2 //GL_PHONG_WIN
	float3 out vNormal : TEXCOORD2,
	float3 out vEcPosition : TEXCOORD3,
	float4 out vDiffuse : TEXCOORD4,
	float4 out vSpecular : TEXCOORD5,
	float4 out vEmission : TEXCOORD6,
#endif
	float4 out vPosition : POSITION,
#if has_colors == 1
	float4 out vColor : COLOR,
#endif
	float out psize : PSIZE,
#if clip_planes_num > 0
	float out vClip[clip_planes_num] : CLP0,
	uniform float4 clip_planes_eq[clip_planes_num],
#endif
	uniform float4x4 modelview,
	uniform float4x4 wvp,
	uniform float4x4 texmat,
	uniform float point_size,
	uniform float4x4 normal_mat
) {
	float4 pos4 = float4(position, 1.f);
	float4 modelpos = mul(modelview, pos4);
	
	// User clip planes
#if clip_planes_num > 0
	for (int i = 0; i < clip_planes_num; i++) {
		vClip[i] = dot(modelpos, clip_planes_eq[i]);
	}
#endif
	vPosition = mul(wvp, pos4);
	
	// Lighting
#if lights_num > 0
	float3 normal = normalize(mul(float3x3(normal_mat), normals));
	float3 ecPosition = modelpos.xyz / modelpos.w;
#if shading_mode == 1 // GL_SMOOTH
	for (int i = 0; i < lights_num; i++) {
		calculate_light(i, ecPosition, normal);
	}
#endif
#endif

#if num_textures > 0
	vTexcoord = mul(texmat, float4(texcoord0, 0.f, 1.f)).xy;
#if num_textures > 1
	vTexcoord2 = mul(texmat, float4(texcoord1, 0.f, 1.f)).xy;
#endif
#endif
#if has_colors == 1
#if lights_num > 0
#if shading_mode == 1 // GL_SMOOTH
	vColor = emission + color * float4(0.2f, 0.2f, 0.2f, 1.0f); // TODO: glLightAmbient impl
	vColor += Ambient * color + Diffuse * diff + Specular * spec;
	vColor = clamp(vColor, 0.0f, 1.0f);
#endif
#if shading_mode == 2 // GL_PHONG_WIN
	vColor = color;
	vNormal = normal;
	vEcPosition = ecPosition;
	vDiffuse = diff;
	vSpecular = spec;
	vEmission = emission;
#endif
#else
	vColor = color;
#endif
#endif
	psize = point_size;
}
)";
