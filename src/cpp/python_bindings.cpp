#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include "particle_renderer.h"

namespace py = pybind11;
glm::mat4 numpy_to_mat4(py::array_t<float> arr) {
    auto buf = arr.request();
    if (buf.ndim != 2 || buf.shape[0] != 4 || buf.shape[1] != 4) {
        throw std::runtime_error("Matrix must be 4x4");
    }
    float* ptr = static_cast<float*>(buf.ptr);
    glm::mat4 mat;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            mat[j][i] = ptr[i * 4 + j];  
        }
    }
    return mat;
}
PYBIND11_MODULE(particle_renderer, m) {
    m.doc() = "High-performance GPU particle renderer with SSBO optimization";
    
    py::class_<particle::ParticleRenderer>(m, "ParticleRenderer")
        .def(py::init<int>(), py::arg("particle_count") = 20000,
             "Create particle renderer with specified particle count")
        
        .def("initialize", &particle::ParticleRenderer::initialize,
             "Initialize OpenGL resources")
        
        .def("cleanup", &particle::ParticleRenderer::cleanup,
             "Clean up OpenGL resources")
        
        .def("update_particles", &particle::ParticleRenderer::updateParticles,
             py::arg("delta_time"),
             "Update particle system state")
        
        .def("reset_particles", &particle::ParticleRenderer::resetParticles,
             "Reset all particles to initial positions")
        
        .def("render", [](particle::ParticleRenderer& self, 
                         py::array_t<float> proj, 
                         py::array_t<float> view, 
                         py::array_t<float> model) {
            glm::mat4 projection = numpy_to_mat4(proj);
            glm::mat4 viewMat = numpy_to_mat4(view);
            glm::mat4 modelMat = numpy_to_mat4(model);
            self.render(projection, viewMat, modelMat);
        }, py::arg("projection"), py::arg("view"), py::arg("model"),
           "Render particles with transformation matrices")
        
        .def("set_wave_amplitude", &particle::ParticleRenderer::setWaveAmplitude,
             py::arg("amplitude"), "Set wave displacement amplitude")
        
        .def("set_wave_frequency", &particle::ParticleRenderer::setWaveFrequency,
             py::arg("frequency"), "Set wave oscillation frequency")
        
        .def("set_curl_strength", &particle::ParticleRenderer::setCurlStrength,
             py::arg("strength"), "Set curl noise strength for trails")
        
        .def("set_curl_persistence", &particle::ParticleRenderer::setCurlPersistence,
             py::arg("persistence"), "Set curl noise persistence (detail level 0.0-1.0)")
        
        .def("set_rotation_speed", &particle::ParticleRenderer::setRotationSpeed,
             py::arg("speed"), "Set rotation speed")
        
        .def("set_glow_intensity", &particle::ParticleRenderer::setGlowIntensity,
             py::arg("intensity"), "Set glow intensity")
        
        .def("set_glow_color", &particle::ParticleRenderer::setGlowColor,
             py::arg("r"), py::arg("g"), py::arg("b"),
             "Set glow color (RGB values 0-1)")
        
        .def("set_audio_reactivity", &particle::ParticleRenderer::setAudioReactivity,
             py::arg("intensity"), "Set audio reactivity intensity (0.0 to 1.0)")
        
        .def("get_particle_count", &particle::ParticleRenderer::getParticleCount,
             "Get total particle count")

        .def("get_time", &particle::ParticleRenderer::getTime,
             "Get current simulation time");
}