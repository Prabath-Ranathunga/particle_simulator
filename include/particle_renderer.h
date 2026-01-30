#ifndef PARTICLE_RENDERER_H
#define PARTICLE_RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

namespace particle {

// Particle structure aligned for SSBO (16-byte alignment)
struct Particle {
    glm::vec3 position;
    float life;         
    glm::vec3 velocity;
    float padding;       
};
class ParticleRenderer {
public:
    ParticleRenderer(int particleCount = 20000);
    ~ParticleRenderer();
    bool initialize();
    void cleanup();
    void updateParticles(float deltaTime);
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void resetParticles();
    void setWaveAmplitude(float amplitude) { m_waveAmplitude = amplitude; }
    void setWaveFrequency(float frequency) { m_waveFrequency = frequency; }
    void setCurlStrength(float strength) { m_curlStrength = strength; }
    void setCurlPersistence(float persistence) { m_curlPersistence = persistence; }
    void setRotationSpeed(float speed) { m_rotationSpeed = speed; }
    void setGlowIntensity(float intensity) { m_glowIntensity = intensity; }
    void setGlowColor(float r, float g, float b) { m_glowColor = glm::vec3(r, g, b); }
    void setAudioReactivity(float intensity) { m_audioReactivity = glm::clamp(intensity, 0.0f, 1.0f); }
    int getParticleCount() const { return m_particleCount; }
    float getTime() const { return m_time; }

private:
    void initializeParticles();
    void setupBuffers();
    bool loadShaders();
    GLuint compileShader(const char* source, GLenum type);
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
    int m_particleCount;
    std::vector<Particle> m_particles;
    GLuint m_vao;
    GLuint m_ssbo;              
    GLuint m_shaderProgram;
    GLuint m_gpuShaderProgram;  
    float m_time;
    float m_lastDeltaTime;
    float m_waveAmplitude;
    float m_waveFrequency;
    float m_curlStrength;
    float m_curlPersistence;
    float m_rotationSpeed;
    float m_glowIntensity;
    glm::vec3 m_glowColor;
    float m_trailSpread;
    float m_audioReactivity; 
};
} 
#endif 