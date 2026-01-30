#version 430 core

// SSBO structure for particles - must align to 16 bytes (vec4)
struct Particle {
    vec3 position;
    float life;
    vec3 velocity;
    float padding;
};

layout (std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform mat4 u_projection;
uniform mat4 u_view;
uniform float u_time;
uniform float u_deltaTime;
uniform float u_audioReactivity;  // Audio reactivity intensity (0.0 to 1.0)

out float v_life;

// Simplex noise permutation table
const int perm[512] = int[512](
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
);

// 4D Simplex Noise
float noise4D(vec4 v) {
    const float F4 = 0.309016994;
    const float G4 = 0.138196601;
    
    float s = (v.x + v.y + v.z + v.w) * F4;
    vec4 skewed = floor(v + s);
    
    float t = (skewed.x + skewed.y + skewed.z + skewed.w) * G4;
    vec4 unskewed = skewed - t;
    vec4 x0 = v - unskewed;
    
    int c1 = (x0.x > x0.y) ? 32 : 0;
    int c2 = (x0.x > x0.z) ? 16 : 0;
    int c3 = (x0.y > x0.z) ? 8 : 0;
    int c4 = (x0.x > x0.w) ? 4 : 0;
    int c5 = (x0.y > x0.w) ? 2 : 0;
    int c6 = (x0.z > x0.w) ? 1 : 0;
    int c = c1 + c2 + c3 + c4 + c5 + c6;
    
    ivec4 i1, i2, i3;
    i1.x = (c >= 32) ? 1 : 0; i1.y = (c >= 16) ? 1 : 0; i1.z = (c >= 8) ? 1 : 0; i1.w = (c >= 4) ? 1 : 0;
    i2.x = (c >= 16) ? 1 : 0; i2.y = (c >= 8) ? 1 : 0; i2.z = (c >= 4) ? 1 : 0; i2.w = (c >= 2) ? 1 : 0;
    i3.x = (c >= 8) ? 1 : 0; i3.y = (c >= 4) ? 1 : 0; i3.z = (c >= 2) ? 1 : 0; i3.w = (c >= 1) ? 1 : 0;
    
    vec4 x1 = x0 - vec4(i1) + G4;
    vec4 x2 = x0 - vec4(i2) + 2.0 * G4;
    vec4 x3 = x0 - vec4(i3) + 3.0 * G4;
    vec4 x4 = x0 - 1.0 + 4.0 * G4;
    
    ivec4 ii = ivec4(skewed) & 255;
    
    int gi0 = perm[ii.x + perm[ii.y + perm[ii.z + perm[ii.w]]]] & 31;
    int gi1 = perm[ii.x + i1.x + perm[ii.y + i1.y + perm[ii.z + i1.z + perm[ii.w + i1.w]]]] & 31;
    int gi2 = perm[ii.x + i2.x + perm[ii.y + i2.y + perm[ii.z + i2.z + perm[ii.w + i2.w]]]] & 31;
    int gi3 = perm[ii.x + i3.x + perm[ii.y + i3.y + perm[ii.z + i3.z + perm[ii.w + i3.w]]]] & 31;
    int gi4 = perm[ii.x + 1 + perm[ii.y + 1 + perm[ii.z + 1 + perm[ii.w + 1]]]] & 31;
    
    float n0 = 0.6 - dot(x0, x0);
    n0 = (n0 < 0.0) ? 0.0 : (n0 * n0 * n0 * n0 * float(gi0));
    
    float n1 = 0.6 - dot(x1, x1);
    n1 = (n1 < 0.0) ? 0.0 : (n1 * n1 * n1 * n1 * float(gi1));
    
    float n2 = 0.6 - dot(x2, x2);
    n2 = (n2 < 0.0) ? 0.0 : (n2 * n2 * n2 * n2 * float(gi2));
    
    float n3 = 0.6 - dot(x3, x3);
    n3 = (n3 < 0.0) ? 0.0 : (n3 * n3 * n3 * n3 * float(gi3));
    
    float n4 = 0.6 - dot(x4, x4);
    n4 = (n4 < 0.0) ? 0.0 : (n4 * n4 * n4 * n4 * float(gi4));
    
    return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

// Curl noise
vec3 curlNoise(vec3 p, float time) {
    const float eps = 0.01;
    const float scale = 1.0;
    const int octaves = 2;
    const float persistence = 0.5;
    
    vec3 curl = vec3(0.0);
    float amplitude = 1.0;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        vec3 p_freq = p * frequency * scale;
        
        float dx_y = noise4D(vec4(p_freq.x, p_freq.y + eps, p_freq.z, time)) - 
                     noise4D(vec4(p_freq.x, p_freq.y - eps, p_freq.z, time));
        float dx_z = noise4D(vec4(p_freq.x, p_freq.y, p_freq.z + eps, time)) - 
                     noise4D(vec4(p_freq.x, p_freq.y, p_freq.z - eps, time));
        
        float dy_x = noise4D(vec4(p_freq.x + eps, p_freq.y, p_freq.z, time)) - 
                     noise4D(vec4(p_freq.x - eps, p_freq.y, p_freq.z, time));
        float dy_z = noise4D(vec4(p_freq.x, p_freq.y, p_freq.z + eps, time)) - 
                     noise4D(vec4(p_freq.x, p_freq.y, p_freq.z - eps, time));
        
        float dz_x = noise4D(vec4(p_freq.x + eps, p_freq.y, p_freq.z, time)) - 
                     noise4D(vec4(p_freq.x - eps, p_freq.y, p_freq.z, time));
        float dz_y = noise4D(vec4(p_freq.x, p_freq.y + eps, p_freq.z, time)) - 
                     noise4D(vec4(p_freq.x, p_freq.y - eps, p_freq.z, time));
        
        curl += amplitude * vec3(dz_y - dy_z, dx_z - dz_x, dy_x - dx_y) / (2.0 * eps);
        
        amplitude *= persistence;
        frequency *= 2.0;
    }
    
    return curl * 0.3;
}

void main() {
    uint idx = uint(gl_VertexID);
    uint numParticles = particles.length();
    
    vec3 pos = particles[idx].position;
    vec3 vel = particles[idx].velocity;
    float life = particles[idx].life;
    
    // Spiral parameters - matched to original CPU implementation
    const float spiralSpeed = 0.15;
    const float targetRadius = 3.2;  // Extended radius for longer trails
    
    // Compute spiral axes
    float angle = atan(pos.z, pos.x);
    float elevation = atan(pos.y, length(pos.xz));
    
    vec3 axis1 = normalize(vec3(-sin(angle), 0.0, cos(angle)));
    vec3 axis2 = normalize(cross(normalize(pos), axis1));
    
    // Organized pressure zones
    float phase = angle * 3.0 + elevation * 2.0 - u_time * 0.3;
    float pressure = sin(phase) * 0.5 + 0.5;
    
    // Add secondary slower wave for more variation
    pressure += sin(u_time * 0.6 + angle * 2.0) * 0.3;
    
    float targetR = targetRadius + pressure * 0.3;
    
    // Dual-axis spiraling - add small per-particle variation to break synchronization
    float particleOffset = float(idx) * 0.001;  // Very small offset to maintain coherence
    float spiralPhase = u_time * 0.3 + angle * 2.0 + particleOffset;
    vec3 spiralVel = axis1 * 5.0 + axis2 * sin(spiralPhase) * 2.5;
    
    // Curl noise - PRIMARY force for trail formation
    // Slower time evolution for smooth, gradual path changes
    // Audio reactivity modulates curl strength (range: 8.0 to 16.0)
    float curlStrengthBase = 8.0;
    float curlStrengthAudio = curlStrengthBase + (u_audioReactivity * 8.0);
    vec3 curlVel = curlNoise(pos * 0.15, u_time * 0.1) * curlStrengthAudio;
    
    // Wave displacement - subtle variation per particle
    float waveAmp = 0.3;
    float waveX = sin(elevation * 2.0 + u_time * 1.5 + particleOffset * 10.0) * waveAmp * 0.5;
    float waveY = cos(angle * 1.5 + u_time * 1.2 + particleOffset * 7.0) * waveAmp * 0.5;
    float waveZ = sin(angle * 2.0 + elevation + u_time * 1.3 + particleOffset * 13.0) * waveAmp * 0.5;
    vec3 wave = vec3(waveX, waveY, waveZ);
    
    // Combine forces - CURL DOMINATES for visible trail formation
    // Pressure creates density variations, curl creates swirling trails
    vec3 pressureForce = normalize(pos) * (pressure - 0.6) * 2.0;  // Radial push/pull for density
    vec3 acceleration = curlVel * 0.5 + spiralVel * 0.2 + wave * 0.15 + pressureForce;
    
    // Smooth sphere containment using inverse square law
    float dist = length(pos);
    const float sphereRadius = 1.5;  // Target sphere radius - reduced for tighter containment
    
    vec3 containmentForce = vec3(0.0);
    if (dist > 0.01) {
        vec3 toCenter = -normalize(pos);
        
        // Smooth containment force that increases with distance from center
        // Creates natural sphere shape without hard boundaries
        float distRatio = dist / sphereRadius;
        
        if (distRatio > 1.0) {
            // Outside sphere: strong pull back
            float overshoot = distRatio - 1.0;
            containmentForce = toCenter * (25.0 * overshoot * overshoot);  // Increased from 15.0
            // Also dampen velocity when outside
            vel *= 0.90;  // Stronger damping from 0.95
        } else if (distRatio > 0.85) {
            // Near edge: gentle pull
            float edgeFactor = (distRatio - 0.85) / 0.15;
            containmentForce = toCenter * (8.0 * edgeFactor);  // Increased from 5.0
        }
        
        // Inner sphere repulsion to prevent clustering at center
        if (distRatio < 0.3) {
            containmentForce = -toCenter * (0.3 - distRatio) * 3.0;
        }
    }
    
    // Add containment to acceleration
    acceleration += containmentForce;
    
    // Update velocity with lighter damping for longer trails
    vel += acceleration * u_deltaTime;
    vel *= 0.985;  // Lighter damping
    
    // Limit velocity for stability
    float maxSpeed = 1.8;
    float speed = length(vel);
    if (speed > maxSpeed) {
        vel = (vel / speed) * maxSpeed;
    }
    
    // Update position
    pos += vel * u_deltaTime * spiralSpeed;
    
    // Keep life constant - no disappearing/reappearing particles
    // Just use life for visual variation with subtle per-particle phase
    life = 0.3 + 0.7 * (sin(u_time * 0.5 + float(idx) * 0.001 * 6283.18) * 0.5 + 0.5);
    
    // Write back to SSBO
    particles[idx].position = pos;
    particles[idx].velocity = vel;
    particles[idx].life = life;
    
    // Output
    gl_Position = u_projection * u_view * vec4(pos, 1.0);
    
    // Calculate point size
    vec4 viewPos = u_view * vec4(pos, 1.0);
    float distance = length(viewPos.xyz);
    gl_PointSize = (20.0 / distance) * 8.0 * (0.5 + life * 0.5);
    
    v_life = life;
}
