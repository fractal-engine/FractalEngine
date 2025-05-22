vec3 a_position  : POSITION;
vec2 a_texcoord0 : TEXCOORD0;   


vec2 v_out_uv          : TEXCOORD1; 
vec3 v_out_worldPos    : TEXCOORD2; 
vec4 v_out_shadowCoord : TEXCOORD3; 
vec3 v_out_viewVec     : TEXCOORD4; 


vec3 v_out_worldTangent   : TEXCOORD5; 
vec3 v_out_worldBitangent : TEXCOORD6; 
vec3 v_out_worldNormalGeom: TEXCOORD7; 


vec3 v_sys_normal    : NORMAL    = vec3(0.0, 0.0, 1.0);
vec3 v_sys_tangent   : TANGENT   = vec3(1.0, 0.0, 0.0);
vec3 v_sys_bitangent : BINORMAL  = vec3(0.0, 1.0, 0.0);
vec4 v_sys_color0    : COLOR     = vec4(1.0, 0.0, 0.0, 1.0);