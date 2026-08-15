uniform sampler2D u_TextureMap;
uniform vec4      u_Color;
uniform float     u_Gamma;
uniform float     u_Intensity;

varying vec2      var_TexCoords;

void main()
{
	vec4 color = texture2D(u_TextureMap, var_TexCoords);

	// Apply intensity scaling (r_intensity, defaults to 1.0)
	color.rgb *= u_Intensity;

	// Apply gamma correction: output = pow(input, 1.0 / gamma)
	// r_gamma ranges 0.5 to 3.0; values > 1.0 brighten, < 1.0 darken.
	// Clamp gamma to avoid division by zero.
	float invGamma = 1.0 / max(u_Gamma, 0.01);
	color.rgb = pow(max(color.rgb, vec3(0.0)), vec3(invGamma));

	// Add ordered dithering to reduce banding from 8-bit quantization.
	// Uses a 4x4 Bayer matrix approximation via interleaved gradient noise.
	// This is the same technique used by the tonemap shader.
	color.rgb += vec3(1.0/510.0 * mod(gl_FragCoord.x + gl_FragCoord.y, 2.0) - 1.0/1020.0);

	gl_FragColor = color * u_Color;
}
