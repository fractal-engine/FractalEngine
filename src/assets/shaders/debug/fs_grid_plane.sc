$input v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_viewPos;

void main()
{
    vec3 worldPos = v_worldPos;
    
    // Calculate distance from camera for fog
    float dist = length(worldPos.xz - u_viewPos.xz);
    float fadeStart = 50.0;
    float fadeEnd = 200.0;
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    
    // LOD: Fine grid (1 unit) and coarse grid (10 units)
    vec2 derivative = fwidth(worldPos.xz);
    
    // Fine grid (1 unit)
    vec2 fineGrid = abs(fract(worldPos.xz - 0.5) - 0.5) / derivative;
    float fineLine = min(fineGrid.x, fineGrid.y);
    float fineAlpha = 1.0 - min(fineLine, 1.0);
    
    // Coarse grid (10 unit)
    vec2 coarseCoord = worldPos.xz / 10.0;
    vec2 coarseGrid = abs(fract(coarseCoord - 0.5) - 0.5) / fwidth(coarseCoord);
    float coarseLine = min(coarseGrid.x, coarseGrid.y);
    float coarseAlpha = 1.0 - min(coarseLine, 1.0);
    
    // Blend between LODs based on zoom level
    float lodFade = smoothstep(1.0, 10.0, derivative.x * 100.0);
    float gridAlpha = mix(fineAlpha * 0.3, coarseAlpha * 0.6, lodFade);
    
    // Axis lines
    float axisThickness = 0.05;
    vec2 axisPos = abs(worldPos.xz);
    float axisX = 1.0 - smoothstep(0.0, axisThickness, axisPos.x);
    float axisZ = 1.0 - smoothstep(0.0, axisThickness, axisPos.y);
    
    // Colors
    vec3 gridColor = vec3(0.3, 0.3, 0.3);
    vec3 axisXColor = vec3(1.0, 0.3, 0.3); // Red X
    vec3 axisZColor = vec3(0.3, 0.3, 1.0); // Blue Z
    
    // Combine
    vec3 color = gridColor;
    color = mix(color, axisZColor, axisZ);
    color = mix(color, axisXColor, axisX);
    
    float alpha = max(gridAlpha, max(axisX, axisZ)) * fade;
    
    gl_FragColor = vec4(color, alpha);
}