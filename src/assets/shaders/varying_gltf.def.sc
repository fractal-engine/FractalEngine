vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec4 a_tangent   : TANGENT;    
vec2 a_texcoord0 : TEXCOORD0;


vec2 v_uv            : TEXCOORD0;
vec3 v_worldPos      : TEXCOORD1;
vec3 v_worldNormal   : TEXCOORD2;
vec3 v_worldTangent  : TEXCOORD3;
vec3 v_worldBitangent: TEXCOORD4;
vec4 v_shadowCoord   : TEXCOORD5;