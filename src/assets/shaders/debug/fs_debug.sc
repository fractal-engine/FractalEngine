$input v_worldPos

#include <bgfx_shader.sh>

void main()
{
    vec3 worldPos = v_worldPos;
    
    // Grid parameters
    float gridSize = 1.0;
    float fadeStart = 20.0;
    float fadeEnd = 50.0;
    
    // Calculate distance for fading
    float dist = length(worldPos);
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    
    // Calculate grid lines on XZ plane
    vec2 coord = worldPos.xz / gridSize;
    vec2 derivative = fwidth(coord);
    vec2 gridUV = abs(fract(coord - 0.5) - 0.5) / derivative;
    float lineValue = min(gridUV.x, gridUV.y);
    
    // Create grid line with anti-aliasing
    float gridLine = 1.0 - min(lineValue, 1.0);
    
    // Axis lines (thicker)
    float axisThickness = 0.05;
    vec2 axisPos = abs(worldPos.xz);
    float axisX = 1.0 - smoothstep(0.0, axisThickness, axisPos.x);
    float axisZ = 1.0 - smoothstep(0.0, axisThickness, axisPos.y);
    
    // Colors
    vec3 gridColor = vec3(0.3, 0.3, 0.3);
    vec3 axisXColor = vec3(1.0, 0.2, 0.2); // Red for X axis
    vec3 axisZColor = vec3(0.2, 0.2, 1.0); // Blue for Z axis
    
    // Combine colors
    vec3 color = gridColor * gridLine;
    color = mix(color, axisZColor, axisZ * 0.8);
    color = mix(color, axisXColor, axisX * 0.8);
    
    // Calculate final alpha with fade
    float combinedAlpha = max(gridLine, max(axisX, axisZ));
    float alpha = combinedAlpha * fade;
    
    gl_FragColor = vec4(color, alpha);
}