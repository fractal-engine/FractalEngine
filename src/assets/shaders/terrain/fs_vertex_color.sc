$input v_color0, v_normal, v_worldPos
#ifdef HAS_TEXCOORDS
$input v_texcoord0
#endif

#include <bgfx_shader.sh>

uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;

void main()
{
    // ============================================
    // Use smoothly blended biome color from vertex
    // ============================================
    vec3 biome_color = v_color0.rgb;
    
    // ============================================
    // Lighting (simple diffuse + ambient)
    // ============================================
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_sunDirection.xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);
    
    // Sky-based ambient (brighter on upward faces)
    float skyFactor = normal.y * 0.5 + 0.5;
    vec3 ambient = mix(vec3(0.3), vec3(0.5), skyFactor);
    
    // Diffuse lighting
    vec3 diffuse = ndotl * u_sunLuminance.rgb * 0.6;
    
    // Combine
    vec3 lit_color = biome_color * (diffuse + ambient);
    
    // ============================================
    // Tone mapping & gamma correction
    // ============================================
    lit_color = lit_color / (lit_color + vec3(1.0));  // Reinhard
    lit_color = pow(lit_color, vec3(1.0 / 2.2));      // Gamma
    
    gl_FragColor = vec4(lit_color, 1.0);
}