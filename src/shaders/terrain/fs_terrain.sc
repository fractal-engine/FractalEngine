$input v_position, v_texcoord0

#include "../common/common.sh"

SAMPLER2D(s_diffuse,  0);
SAMPLER2D(s_orm,      1);
SAMPLER2D(s_normal,   2);

uniform vec4 u_sunDirection;  // xyz = sun dir
uniform vec4 u_sunLuminance;  // rgb = color
uniform vec4 u_cameraPos;

void main()
{
    vec2 uv = v_texcoord0;

    // --- Sample maps
    vec3 albedo     = texture2D(s_diffuse, uv).rgb;
    vec3 orm        = texture2D(s_orm, uv).rgb;
    vec3 normalMap  = texture2D(s_normal, uv).rgb * 2.0 - 1.0;

    float ao        = orm.r;
    float roughness = orm.g;
    float metalness = orm.b;

    vec3 normal     = normalize(normalMap);
    vec3 lightColor = u_sunLuminance.rgb;  //  dynamic sun color
    vec3 lightDir = normalize(u_sunDirection.xyz); //  dynamic sun dir

    vec3 viewDir    = normalize(u_cameraPos.xyz - v_position);
    vec3 halfVec    = normalize(lightDir + viewDir);

    float NdotL     = max(dot(normal, lightDir), 0.0);
    float specPower = mix(4.0, 64.0, 1.0 - roughness);
    float spec      = pow(max(dot(normal, halfVec), 0.0), specPower);


    vec3 diffuse    = albedo * lightColor * NdotL * ao;
    vec3 specular   = lightColor * spec * (1.0 - metalness);

    vec3 finalColor = diffuse + specular;

    gl_FragColor = vec4(finalColor, 1.0);
}
