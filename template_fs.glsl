#version 430
#define	str_K  0.1
#define	str_Kf 1.12
#define	str_Ks 1.01
layout(binding = 0) uniform sampler2D diffuse_tex; 
layout(location = 1) uniform float time;
layout(location = 2) uniform float m;
layout(location = 3) uniform float n;
layout(location = 4) uniform vec4 ka;
layout(location = 5) uniform vec4 kd;
layout(location = 6) uniform vec4 ks;
layout(location = 7) uniform int o;
layout(location = 8) uniform int lighting_models;
layout(location = 9) uniform float nu;
layout(location = 10) uniform float nv;
layout(location = 11) uniform int  spec;
layout(std140, binding = 0) uniform SceneUniforms
{
   mat4 PV;	//camera projection * view matrix
   vec4 eye_w;	//world-space eye position
};

layout(std140, binding = 1) uniform LightUniforms
{
   vec4 La;	//ambient light color
   vec4 Ld;	//diffuse light color
   vec4 Ls;	//specular light color
   vec4 light_w; //world-space light position
};

layout(std140, binding = 2) uniform MaterialUniforms
{
   //vec4 ka;	//ambient material color
   //vec4 kd;	//diffuse material color
   //vec4 ks;	//specular material color
   float shininess; //specular exponent
};

in VertexData
{
   vec2 tex_coord;
   vec3 pw;       //world-space vertex position
   vec3 nw;   //world-space normal vector
} inData;   //block is named 'inData'

out vec4 fragcolor; //the output color for this fragment    
// Shadowing-masking function (Smith with Beckmann distribution)
float smithG1(float cosTheta, float alpha) {

	if (cosTheta == 0) {
		cosTheta = 0.001;
	}

	float tanTheta = sqrt(1.0 - cosTheta * cosTheta) / cosTheta;
	float a_deno = alpha * tanTheta;
	if (a_deno == 0) {
		a_deno = 0.001;
	}
	float a = 1.0 / a_deno;
	if (a >= 1.6) {
		return 1.0;
	}
	return (3.535 * a + 2.181 * a * a) / (1.0 + 2.276 * a + 2.577 * a * a);
}

// Strauss model defines its' own fresnel
float strFresnel(float x, float kf)
{
	float	dx = x - kf;
	float	d1 = 1.0 - kf;
	float	kf2 = kf * kf;

	return (1.0 / (dx * dx) - 1.0 / kf2) /
		(1.0 / (d1 * d1) - 1.0 / kf2);
}
float strShadow(float x, float ks)
{

	float	dx = x - ks;
	float	d1 = 1.0 - ks;
	float	ks2 = ks * ks;

	return (1.0 / (dx * dx) - 1.0 / ks2) / (1.0 / (d1 * d1) - 1.0 / ks2);
}

void main(void)
{
	//Compute per-fragment Phong lighting
	vec4 ktex = texture(diffuse_tex, inData.tex_coord);

	vec4 ambient_term = ka * La;

	const float eps = 1e-8; //small value to avoid division by 0
	float d = distance(light_w.xyz, inData.pw.xyz);
	float atten = 1.0 / (d * d + eps); //d-squared attenuation

	vec3 nw = normalize(inData.nw);			//world-space unit normal vector
	vec3 lw = normalize(light_w.xyz - inData.pw.xyz);	//world-space unit light vector
	vec4 diffuse_term = atten * kd * Ld * max(0.0, dot(nw, lw));

	vec3 vw = normalize(eye_w.xyz - inData.pw.xyz);	//world-space unit view vector
	vec3 rw = reflect(-lw, nw);	//world-space unit reflection vector
	vec3 h = (lw + vw) / length(lw + vw);
	h = normalize(h);

	vec3  tangent = vec3(0.0, 0.0, 0.0);
	vec3 bitangent = vec3(0.0, 0.0, 0.0);

	if (nw == vec3(0.0, 1.0, 0.0)) {
		tangent = vec3(1.0, 0.0, 0.0);
	}
	else {
		tangent = normalize(cross(vec3(0.0, 1.0, 0.0), nw));

	}
	bitangent = normalize(cross(nw, tangent));

	float hu = dot(h, tangent);
	float hv = dot(h, bitangent);




	float pi = 3.141592;
	float n_deno = n + 1;
	float n_nume = 1 - n;
	if (n == -1) {
		float n_deno = 1;
	}
	if (n == 1) {
		float n_nume = 1;
	}

	float f0 = pow(n_nume / n_deno, 2);

	float f = f0 + (1 - f0) * pow(1 - max(dot(nw, vw), 0), 5);
	float fA = f0 + (1 - f0) * pow(1 - max(dot(h, vw), 0), 5);//fresnal for shirley lighting



	float cosalpha = pow(max(dot(nw, h), 0), 4);

	float tanalpha_deno = pow(dot(nw, h), 2);
	if (tanalpha_deno == 0) {
		tanalpha_deno = 0.0001;
	}

	float tanalpha = (1 - pow(max(dot(nw, h), 0), 2)) / tanalpha_deno;

	float drough_deno = 4 * pow(m, 2) * cosalpha;
	if (drough_deno == 0) {
		drough_deno = 0.0001;
	}

	float drough = exp(-1 * (tanalpha / pow(m, 2))) / drough_deno;

	float masking_deno = dot(vw, h);
	if (masking_deno == 0) {
		masking_deno = 0.0001;
	}

	float masking = 2 * (max(dot(nw, h), 0) * max(dot(nw, vw), 0)) / masking_deno;
	float shadow_deno = dot(vw, h);
	if (shadow_deno == 0) {
		shadow_deno = 0.001;
	}
	float shadowing = 2 * (max(dot(nw, h), 0) * max(dot(nw, lw), 0)) / shadow_deno;
	float x = min(masking, shadowing);
	float g = min(1, x);

	float ct = (f * drough * g) / pi * dot(nw, vw);
	// Trowbridge - Reitz
	 //float tr = F * G * D / (4.0 * NdotV * NdotL);
	float cosThetaV = max(dot(nw, vw), 0.001);
	float cosThetaL = max(dot(nw, lw), 0.001);
	float cosThetaH = max(dot(nw, h), 0.001);
	float cosThetaD = max(dot(h, vw), 0.001); // Dot product between half vector and view direction


	float gt = pow(cosThetaH, 2) * ((m * m) - 1) + 1;
	float d_ggx = (m * m) / pi * pow(gt, 2);

	float G = smithG1(cosThetaV, m) * smithG1(cosThetaL, m);
	float tr_deno = 4.0 * max(dot(nw, lw), 0.01) * max(dot(nw, vw), 0.01);


	float tr = (f * G * d_ggx) / tr_deno;


	///Ashikhmin-Shirley model
	float ndh = max(0, dot(nw, h));
	float ndv = max(0, dot(nw, vw));
	float hdv = max(0, dot(h, vw));
	float ndl = max(0, dot(nw, lw));

	//isotropic specular term
	//nv and nu are the iso and aniso terms which were used in the paper to focus th elight accordingly
   /* float specular_iso = ((nu+1)*pow(ndh,nu))/ (8 * 3.14159265 * hdv * max(ndl, ndv));
	float specular_aniso = ((nu + 1) * pow(dot(h, vw), nu) + (nv + 1) * pow(dot(h, lw), nv)) / (8 * 3.14159265 * hdv * max(ndl, ndv));*/
	float hdn = max(0, dot(h, nw));
	if (hdn == -1 || hdn == 1) {
		hdn += 0.001;
	}
	float power_deno = (1 - pow(hdn, 2));
	if (power_deno == 0) {
		power_deno += 0.01;
	}
	float power_spec_as = ((nu * pow(hu, 2)) + (nv * pow(hv, 2))) / power_deno;
	float specular_as_deno = (hdv * max(ndv, ndl));
	if (specular_as_deno == 0) {
		specular_as_deno = 0.001;
	}
	float specular_as = ((pow((nu + 1) * (nv + 1), 0.5)) / (8 * pi)) * (pow(ndh, power_spec_as) / max(specular_as_deno, 0.001)) * fA;

	//strauss lighting
	float lambert = max(0.0, dot(nw, lw));

	float smoothness = 1.0 - drough; //later send smoothness and roughness as uniforms

	const float transparency = 0.0;

	vec3 lrn = reflect(lw, nw);
	float hDotV = dot(lrn, vw);
	float fstr = strFresnel(lambert, str_Kf);
	float smoothness3 = pow(smoothness, 3);

	//diffuse term for strauss it is slightly different

	float	omst = (1.0 - smoothness3) * (1.0 - transparency);
	vec4 diff = lambert * (1.0 - shininess * smoothness) * omst * kd;

	//specular for strauss

	float	r = (1.0 - transparency) - omst;
	float	j = fstr * strShadow(lambert, str_Ks) * strShadow(ndv, str_Ks);
	float	refl = min(1.0, r + j * (r + str_K));
	vec4	Cs = vec4(1.0) + shininess * (1.0 - fstr) * (kd - vec4(1.0));
	vec4	spec = Cs * pow(-hDotV, 3.0 / (1.0 - smoothness)) * refl;

	diff = max(vec4(0.0), diff);
	spec = max(vec4(0.0), spec);
	//lommel - seelinger model
	float lommel_deno = lambert + ndv;
	vec4 lommel_diffuse =  kd * lambert / lommel_deno;



	// switch (o) {
	// case 0:
	   //  ct = (f * drough * g) / (pi * dot(nw, vw));
	   //  break;
	// case 1:
	   //  ct = f  / (pi * dot(nw, vw));
	   //  break;
	// case 2:
	   //  ct = drough  / (pi * dot(nw, vw));
	   //  break;
	// case 3:
	   //  ct =  g / (pi * dot(nw, vw));
	   //  break;
	// //case 4:
	   // // //trowbridge - reitz
	   // // tr = 
	// default:
		  ////tr = f * g * d / (4.0 * ndotv * ndotl);
	   //  ct = (f * drough * g) / (pi * dot(nw, vw));
	   //  break;

	// }
	vec4 specular_term = ks * Ls;
	vec4 strauss = diff + spec * ks;


	switch (lighting_models) {
	case 0:
		specular_term = ks * Ls * tr;
		break;

	case 1:
		specular_term = ks * Ls * specular_as;
		break;

	case 2:
		//strauss lighting
		fragcolor = ambient_term + strauss;

		break;
	case 3:
		//lomel-seeliger lighting
		diffuse_term = lommel_diffuse;
		specular_term = vec4(0);
		ambient_term = vec4(0);


		break;
	}

	if (lighting_models != 2) {
		fragcolor = ambient_term + diffuse_term + specular_term;
	}
	
		

}



