#include "particle_renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <chrono>

// 4D Simplex Noise implementation
namespace SimplexNoise {
    // Permutation table
    static const unsigned char perm[512] = {
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
    };
    
    inline float grad(int hash, float x, float y, float z, float w) {
        int h = hash & 31;
        float a = (h < 24) ? x : y;
        float b = (h < 16) ? y : z;
        float c = (h < 8) ? z : w;
        return ((h & 1) ? -a : a) + ((h & 2) ? -b : b) + ((h & 4) ? -c : c);
    }
    
    float noise4D(float x, float y, float z, float w) {
        // Skewing and unskewing factors for 4D
        const float F4 = 0.309016994f; // (sqrt(5) - 1) / 4
        const float G4 = 0.138196601f; // (5 - sqrt(5)) / 20
        
        // Skew the input space to determine which simplex cell we're in
        float s = (x + y + z + w) * F4;
        int i = (int)std::floor(x + s);
        int j = (int)std::floor(y + s);
        int k = (int)std::floor(z + s);
        int l = (int)std::floor(w + s);
        
        float t = (i + j + k + l) * G4;
        float X0 = i - t;
        float Y0 = j - t;
        float Z0 = k - t;
        float W0 = l - t;
        
        float x0 = x - X0;
        float y0 = y - Y0;
        float z0 = z - Z0;
        float w0 = w - W0;
        
        // For 4D case, the simplex is a 4D shape with 5 corners
        int c1 = (x0 > y0) ? 32 : 0;
        int c2 = (x0 > z0) ? 16 : 0;
        int c3 = (y0 > z0) ? 8 : 0;
        int c4 = (x0 > w0) ? 4 : 0;
        int c5 = (y0 > w0) ? 2 : 0;
        int c6 = (z0 > w0) ? 1 : 0;
        int c = c1 + c2 + c3 + c4 + c5 + c6;
        
        // Simplex vertices
        int i1 = (c >= 3) ? 1 : 0;
        int j1 = (c >= 2 && c < 5) ? 1 : 0;
        int k1 = (c >= 1 && c < 4) ? 1 : 0;
        int l1 = (c < 3) ? 1 : 0;
        
        int i2 = (c >= 2) ? 1 : 0;
        int j2 = (c >= 1 && c < 4) ? 1 : 0;
        int k2 = (c < 3) ? 1 : 0;
        int l2 = (c < 2) ? 1 : 0;
        
        int i3 = (c >= 1) ? 1 : 0;
        int j3 = (c < 3) ? 1 : 0;
        int k3 = (c < 2) ? 1 : 0;
        int l3 = (c < 1) ? 1 : 0;
        
        float x1 = x0 - i1 + G4;
        float y1 = y0 - j1 + G4;
        float z1 = z0 - k1 + G4;
        float w1 = w0 - l1 + G4;
        
        float x2 = x0 - i2 + 2.0f * G4;
        float y2 = y0 - j2 + 2.0f * G4;
        float z2 = z0 - k2 + 2.0f * G4;
        float w2 = w0 - l2 + 2.0f * G4;
        
        float x3 = x0 - i3 + 3.0f * G4;
        float y3 = y0 - j3 + 3.0f * G4;
        float z3 = z0 - k3 + 3.0f * G4;
        float w3 = w0 - l3 + 3.0f * G4;
        
        float x4 = x0 - 1.0f + 4.0f * G4;
        float y4 = y0 - 1.0f + 4.0f * G4;
        float z4 = z0 - 1.0f + 4.0f * G4;
        float w4 = w0 - 1.0f + 4.0f * G4;
        
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int ll = l & 255;
        
        float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f, n4 = 0.0f;
        
        float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0 - w0*w0;
        if (t0 > 0.0f) {
            t0 *= t0;
            n0 = t0 * t0 * grad(perm[ii + perm[jj + perm[kk + perm[ll]]]], x0, y0, z0, w0);
        }
        
        float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1 - w1*w1;
        if (t1 > 0.0f) {
            t1 *= t1;
            n1 = t1 * t1 * grad(perm[ii + i1 + perm[jj + j1 + perm[kk + k1 + perm[ll + l1]]]], x1, y1, z1, w1);
        }
        
        float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2 - w2*w2;
        if (t2 > 0.0f) {
            t2 *= t2;
            n2 = t2 * t2 * grad(perm[ii + i2 + perm[jj + j2 + perm[kk + k2 + perm[ll + l2]]]], x2, y2, z2, w2);
        }
        
        float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3 - w3*w3;
        if (t3 > 0.0f) {
            t3 *= t3;
            n3 = t3 * t3 * grad(perm[ii + i3 + perm[jj + j3 + perm[kk + k3 + perm[ll + l3]]]], x3, y3, z3, w3);
        }
        
        float t4 = 0.6f - x4*x4 - y4*y4 - z4*z4 - w4*w4;
        if (t4 > 0.0f) {
            t4 *= t4;
            n4 = t4 * t4 * grad(perm[ii + 1 + perm[jj + 1 + perm[kk + 1 + perm[ll + 1]]]], x4, y4, z4, w4);
        }
        
        return 27.0f * (n0 + n1 + n2 + n3 + n4);
    }
}

// Curl noise for 3D vector fields
glm::vec3 curlNoise(const glm::vec3& p, float time, float persistence) {
    const float eps = 0.01f;
    float freq = 1.0f;
    float amp = 1.0f;
    glm::vec3 curl(0.0f);
    
    // Reduced octaves from 3 to 2 for better performance
    for (int octave = 0; octave < 2; ++octave) {
        // Sample noise at offset positions
        float n1 = SimplexNoise::noise4D(p.x * freq, p.y * freq, p.z * freq + eps, time * 0.5f);
        float n2 = SimplexNoise::noise4D(p.x * freq, p.y * freq, p.z * freq - eps, time * 0.5f);
        float dx = (n1 - n2) / (2.0f * eps);
        
        n1 = SimplexNoise::noise4D(p.x * freq + eps, p.y * freq, p.z * freq, time * 0.5f);
        n2 = SimplexNoise::noise4D(p.x * freq - eps, p.y * freq, p.z * freq, time * 0.5f);
        float dy = (n1 - n2) / (2.0f * eps);
        
        n1 = SimplexNoise::noise4D(p.x * freq, p.y * freq + eps, p.z * freq, time * 0.5f);
        n2 = SimplexNoise::noise4D(p.x * freq, p.y * freq - eps, p.z * freq, time * 0.5f);
        float dz = (n1 - n2) / (2.0f * eps);
        
        // Curl = ∇ × F
        curl.x += (dz - dy) * amp;
        curl.y += (dx - dz) * amp;
        curl.z += (dy - dx) * amp;
        
        freq *= 2.0f;
        amp *= persistence;
    }
    
    return curl;
}

namespace particle {

ParticleRenderer::ParticleRenderer(int particleCount)
    : m_particleCount(particleCount)
    , m_vao(0)
    , m_ssbo(0)
    , m_shaderProgram(0)
    , m_gpuShaderProgram(0)
    , m_time(0.0f)
    , m_lastDeltaTime(0.016f)
    , m_waveAmplitude(1.2f)
    , m_waveFrequency(1.5f)
    , m_curlStrength(2.5f)
    , m_curlPersistence(0.65f)
    , m_rotationSpeed(0.4f)
    , m_glowIntensity(2.5f)
    , m_glowColor(0.2f, 0.5f, 1.0f)
    , m_trailSpread(2.0f)
    , m_audioReactivity(0.0f)
{
    m_particles.resize(m_particleCount);
}

ParticleRenderer::~ParticleRenderer() {
    cleanup();
}

bool ParticleRenderer::initialize() {
    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        return false;
    }
    
    // Check OpenGL version
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 not supported!" << std::endl;
        return false;
    }
    
    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
    
    // Initialize particles
    initializeParticles();
    
    // Setup OpenGL buffers
    setupBuffers();
    
    // Load shaders
    if (!loadShaders()) {
        std::cerr << "Failed to load shaders!" << std::endl;
        return false;
    }
    
    // Enable point sprites
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    std::cout << "Initialized " << m_particleCount << " particles" << std::endl;
    return true;
}

void ParticleRenderer::cleanup() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_ssbo) glDeleteBuffers(1, &m_ssbo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_gpuShaderProgram) glDeleteProgram(m_gpuShaderProgram);
}

void ParticleRenderer::initializeParticles() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> velDist(-0.05f, 0.05f);
    std::uniform_real_distribution<float> lifeDist(0.5f, 1.0f);
    
    const float PI = 3.14159265359f;
    const float sphereRadius = 3.0f;
    
    for (int i = 0; i < m_particleCount; ++i) {
        float t = static_cast<float>(i) / m_particleCount;
        
        // Spherical distribution using fibonacci sphere algorithm
        float phi = std::acos(1.0f - 2.0f * t);
        float theta = PI * (1.0f + std::sqrt(5.0f)) * t;
        
        // Add some randomness to radius for volume distribution
        float r = sphereRadius * std::pow(radiusDist(gen), 0.3f);
        
        m_particles[i].position = glm::vec3(
            r * std::sin(phi) * std::cos(theta),
            r * std::sin(phi) * std::sin(theta),
            r * std::cos(phi)
        );
        
        m_particles[i].velocity = glm::vec3(
            velDist(gen),
            velDist(gen),
            velDist(gen)
        );
        
        m_particles[i].life = lifeDist(gen);
        m_particles[i].padding = 0.0f;  // SSBO alignment padding
    }
}

void ParticleRenderer::setupBuffers() {
    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Create SSBO (Shader Storage Buffer Object) for GPU-side particle updates
    glGenBuffers(1, &m_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                 m_particles.size() * sizeof(Particle), 
                 m_particles.data(), 
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
    
    // Unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);
}

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint ParticleRenderer::compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    
    return shader;
}

GLuint ParticleRenderer::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // Check linking
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program linking failed:\n" << infoLog << std::endl;
        return 0;
    }
    
    return program;
}

bool ParticleRenderer::loadShaders() {
    // Load GPU-optimized particle shaders
    std::string vertexCode = readFile("src/shaders/particle.vert");
    std::string fragmentCode = readFile("src/shaders/particle.frag");
    
    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Failed to read shader files!" << std::endl;
        return false;
    }
    
    GLuint vertexShader = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER);
    
    if (!vertexShader || !fragmentShader) {
        std::cerr << "Failed to compile shaders!" << std::endl;
        return false;
    }
    
    m_gpuShaderProgram = linkProgram(vertexShader, fragmentShader);
    m_shaderProgram = m_gpuShaderProgram;  // Use GPU shader
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (m_gpuShaderProgram == 0) {
        std::cerr << "Failed to link shader program!" << std::endl;
        return false;
    }
    
    std::cout << "Loaded GPU-optimized SSBO shaders" << std::endl;
    return m_gpuShaderProgram != 0;
}

void ParticleRenderer::updateParticles(float deltaTime) {
    // With GPU-based SSBO, all particle updates happen in the vertex shader
    m_time += deltaTime;
    m_lastDeltaTime = deltaTime;
}

void ParticleRenderer::render(const glm::mat4& projection, const glm::mat4& view, 
                              const glm::mat4& model) {
    glUseProgram(m_gpuShaderProgram);
    
    // Set transformation matrices
    GLint projLoc = glGetUniformLocation(m_gpuShaderProgram, "u_projection");
    GLint viewLoc = glGetUniformLocation(m_gpuShaderProgram, "u_view");
    GLint timeLoc = glGetUniformLocation(m_gpuShaderProgram, "u_time");
    GLint deltaTimeLoc = glGetUniformLocation(m_gpuShaderProgram, "u_deltaTime");
    GLint audioReactivityLoc = glGetUniformLocation(m_gpuShaderProgram, "u_audioReactivity");
    
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniform1f(timeLoc, m_time);
    glUniform1f(deltaTimeLoc, m_lastDeltaTime);
    glUniform1f(audioReactivityLoc, m_audioReactivity);
    
    // Enable blending for particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing for transparent particles
    glDepthMask(GL_FALSE);
    
    // Bind SSBO
    glBindVertexArray(m_vao);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
    
    // Render particles
    glDrawArrays(GL_POINTS, 0, m_particleCount);
    
    // Memory barrier to ensure SSBO writes complete before next frame
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    // Unbind
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);
    
    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void ParticleRenderer::resetParticles() {
    initializeParticles();
    setupBuffers();
}
}