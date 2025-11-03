$input v_color0, v_normal, v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_sunDirection.xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);
    
    // Mix between ground and skybox based on normal
    float skyFactor = normal.y * 0.5 + 0.5;  // 0.0 (down) to 1.0 (up)
    vec3 ambient = mix(vec3(0.3), vec3(0.5), skyFactor);  // Darker below, brighter above
    
    // Diffuse lighting with controlled sun contribution
    vec3 diffuse = ndotl * u_sunLuminance.rgb * 0.5;  // Reduce sun intensity by half
    
    // Combine lighting
    vec3 lit_color = v_color0.rgb * (diffuse + ambient);
    
    // Tone mapping (avoid overexposure)
    // Reinhard tone mapping: maps [0, infinity] to [0, 1]
    lit_color = lit_color / (lit_color + vec3(1.0));
    
    // Gamma correction
    lit_color = pow(lit_color, vec3(1.0 / 2.2));
    
    gl_FragColor = vec4(lit_color, 1.0);
}