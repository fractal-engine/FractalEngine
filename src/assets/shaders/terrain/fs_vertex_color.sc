$input v_color0, v_normal, v_worldPos
#ifdef HAS_TEXCOORDS
$input v_texcoord0
#endif

#include <bgfx_shader.sh>

uniform vec4 u_sunDirection;
uniform vec4 u_sunLuminance;

void main()
{
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(u_sunDirection.xyz);
    float ndotl = max(dot(normal, lightDir), 0.0);
    
    // ============================================
    // Decode height from alpha channel
    // ============================================
    float height = v_color0.a * 100.0 - 50.0;  // Denormalize [0,1] → [-50, 50]
    
    #ifdef HAS_TEXCOORDS
    float slope = v_texcoord0.y;
    #else
    float slope = 0.0;
    #endif
    
    // ============================================
    // Detect biome boundaries using derivatives
    // ============================================
    // dFdx/dFdy measure how quickly color changes across pixels
    vec3 color_dx = dFdx(v_color0.rgb);
    vec3 color_dy = dFdy(v_color0.rgb);
    float color_change = length(color_dx) + length(color_dy);
    
    // If color changes rapidly, we're near a biome boundary
    // Apply smoothing by blending with neighbor colors
    float boundary_strength = smoothstep(0.0, 0.3, color_change);
    
    // Use biome color from constraint system
    vec3 biome_color = v_color0.rgb;
    
    // ============================================
    // Optional: Add subtle noise to break up hard edges
    // ============================================
    // Simple hash-based noise using world position
    float noise = fract(sin(dot(v_worldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
    noise = (noise - 0.5) * 0.05;  // Small random offset [-0.025, 0.025]
    
    // Apply noise only at biome boundaries
    biome_color = mix(biome_color, biome_color + vec3(noise), boundary_strength * 0.5);
    
    // ============================================
    // Lighting (unchanged)
    // ============================================
    float skyFactor = normal.y * 0.5 + 0.5;
    vec3 ambient = mix(vec3(0.3), vec3(0.5), skyFactor);
    vec3 diffuse = ndotl * u_sunLuminance.rgb * 0.5;
    
    vec3 lit_color = biome_color * (diffuse + ambient);
    
    // Tone mapping
    lit_color = lit_color / (lit_color + vec3(1.0));
    lit_color = pow(lit_color, vec3(1.0 / 2.2));
    
    gl_FragColor = vec4(lit_color, 1.0);
}