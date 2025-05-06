$input v_position, v_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_diffuse, 0);  // Texture unit 0
SAMPLER2D(s_orm,     1);  // Texture unit 1
SAMPLER2D(s_normal,  2);  // Texture unit 2

uniform vec4 u_sunDirection; // Sun direction from C++

void main()
{
    vec2 uv = v_texcoord0;

    // Sample textures
    vec3 albedo = texture2D(s_diffuse, uv).rgb;
    vec3 orm = texture2D(s_orm, uv).rgb;       // r = AO, g = Roughness, b = Metalness
    vec3 normalMap = texture2D(s_normal, uv).xyz * 2.0 - 1.0;  // unpack normal

    // Simple lighting simulation (basic Lambertian reflectance)
    vec3 normal = normalize(normalMap);
    vec3 lightDir = normalize(u_sunDirection.xyz); // Use dynamic sun direction
    float lightIntensity = max(dot(normal, lightDir), 0.0); // Lambertian lighting

    vec3 color = albedo * lightIntensity;

    gl_FragColor = vec4(color, 1.0);
}
