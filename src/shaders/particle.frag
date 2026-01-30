#version 330 core

in float v_life;
out vec4 FragColor;

void main() {
    // Calculate distance from center of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;  // Normalize to 0-1 range
    
    // Discard pixels outside circle to make round particles
    if (dist > 1.0) {
        discard;
    }
    
    // Bright vivid blue colors with high contrast
    vec3 innerCore = vec3(0.6, 0.9, 1.0);    // Bright cyan-blue center
    vec3 outerGlow = vec3(0.1, 0.5, 1.0);    // Deep electric blue
    
    // Almost no glow - very sharp particles
    float glowRadius = 0.08;  // Tiny glow
    float coreBrightness = pow(1.0 - smoothstep(0.0, 0.08, dist), 4.0);  // Very sharp center
    float glowIntensity = pow(1.0 - smoothstep(0.0, glowRadius, dist), 8.0);  // Extremely sharp falloff
    
    // Mix colors for high contrast gradient
    vec3 color = mix(outerGlow, innerCore, coreBrightness);
    
    // Minimal brightness boost
    color *= (1.0 + coreBrightness * 0.3);
    
    // Extremely tight alpha - almost no halo
    float alpha = pow(1.0 - smoothstep(0.0, 0.5, dist), 5.0) * v_life;  // Extremely sharp cutoff
    
    // Minimal glow contribution
    vec3 finalColor = color * (0.9 + glowIntensity * 0.1);
    
    FragColor = vec4(finalColor, alpha);
}
