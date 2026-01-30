#!/usr/bin/env python3
"""
Particle Simulator - GPU Accelerated Visualization
"""

import sys
import os
import time
import socket
import json
import threading
import numpy as np
from PyQt5.QtWidgets import QApplication, QOpenGLWidget
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QSurfaceFormat
from OpenGL.GL import *

# Import C++ renderer module
try:
    import particle_renderer  # type: ignore
except ImportError:
    print("ERROR: particle_renderer module not found!")
    sys.exit(1)

# Import renderer utilities
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from renderer_utils import Camera


class ParticleWidget(QOpenGLWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        
        # Set OpenGL surface format with alpha channel for transparency
        fmt = QSurfaceFormat()
        fmt.setVersion(3, 3)
        fmt.setProfile(QSurfaceFormat.CoreProfile)
        fmt.setSamples(0)  # Disable MSAA for better performance
        fmt.setAlphaBufferSize(8)
        fmt.setSwapInterval(0)  # Disable VSync for maximum FPS
        fmt.setSwapBehavior(QSurfaceFormat.DoubleBuffer)
        self.setFormat(fmt)
        
        # Enable transparency
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setAttribute(Qt.WA_NoSystemBackground, False)
        
        # C++ renderer instance
        self.renderer = None
        
        # Camera system
        self.camera = Camera()
        self.camera.set_position(0, 8, 20)  # Further back and higher
        self.camera.look_at(0, 0, 0)
        
        # Timing
        self.last_time = time.time()
        
        # Mouse control
        self.mouse_pressed = False
        self.last_mouse_x = 0
        self.last_mouse_y = 0
        
        # Auto-rotation
        self.auto_rotate = True
        self.rotation_angle = 0.0
        
        # Audio reactivity
        self.current_audio_intensity = 0.0
        
        # FPS tracking
        self.fps_counter = 0
        self.fps_time = time.time()
        self.current_fps = 0
        
        # Update timer (~100 FPS target for smooth rendering)
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(10)  # 10ms = ~100 FPS target
        
        print("Particle Widget initialized")
    
    def initializeGL(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(os.path.dirname(script_dir))
        os.chdir(project_root)
        print(f"Working directory: {os.getcwd()}")
        
        self.renderer = particle_renderer.ParticleRenderer(500000)  # 500,000 particles with GPU SSBO
        
        if not self.renderer.initialize():
            print("ERROR: Failed to initialize particle renderer!")
            sys.exit(1)
        
        glClearColor(0.0, 0.0, 0.0, 0.0)  # Transparent background
        glEnable(GL_DEPTH_TEST)  # Enable depth test
        
        self.renderer.set_glow_intensity(10.0)  # Max brightness
        self.renderer.set_glow_color(1.0, 1.0, 1.0)  # Pure white
        
        glViewport(0, 0, self.width(), self.height())
        print(f"Viewport: {self.width()}x{self.height()}")
        
        print(f"C++ Renderer initialized: {self.renderer.get_particle_count()} particles")
        print(f"OpenGL: {glGetString(GL_VERSION).decode()}")
        
        print(f"Camera position: {self.camera.position}")
        print(f"Camera target: {self.camera.target}")
    
    def paintGL(self):
        if not self.renderer:
            return
        current_time = time.time()
        delta_time = current_time - self.last_time
        self.last_time = current_time
        
        # Auto-rotate camera 
        if self.auto_rotate:
            self.rotation_angle += delta_time * 0.3
            radius = 20.0
            self.camera.set_position(
                np.sin(self.rotation_angle) * radius,
                8.0,
                np.cos(self.rotation_angle) * radius
            )
            self.camera.look_at(0, 0, 0)
        
        # Always update particle system 
        self.renderer.update_particles(delta_time)
        
        # Update FPS counter
        self.fps_counter += 1
        if current_time - self.fps_time >= 1.0:
            self.current_fps = self.fps_counter
            self.fps_counter = 0
            self.fps_time = current_time
            print(f"\rFPS: {self.current_fps} INT: {self.current_audio_intensity:.2f}", end='', flush=True)
        
        # Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        
        # Ensure viewport is correct
        glViewport(0, 0, self.width(), self.height())
        
        # Get transformation matrices
        projection = self.camera.get_projection_matrix(
            self.width() / self.height()
        )
        view = self.camera.get_view_matrix()
        model = np.eye(4, dtype=np.float32)
        
        # Render particles (C++ high-performance rendering)
        try:
            self.renderer.render(projection, view, model)
            # Check for errors after rendering
            err = glGetError()
            if err != 0:
                print(f"OpenGL error after render: {err}")
        except Exception as e:
            print(f"Render error: {e}")
    
    def resizeGL(self, w, h):
        """Handle window resize."""
        glViewport(0, 0, w, h)
    
    def mousePressEvent(self, event):
        """Handle mouse press."""
        if event.button() == Qt.LeftButton:
            self.mouse_pressed = True
            self.auto_rotate = False
            self.last_mouse_x = event.x()
            self.last_mouse_y = event.y()
    
    def mouseReleaseEvent(self, event):
        """Handle mouse release."""
        if event.button() == Qt.LeftButton:
            self.mouse_pressed = False
    
    def mouseMoveEvent(self, event):
        """Handle mouse movement for camera control."""
        if self.mouse_pressed:
            dx = event.x() - self.last_mouse_x
            dy = event.y() - self.last_mouse_y
            
            self.camera.rotate(dx * 0.01, dy * 0.01)
            
            self.last_mouse_x = event.x()
            self.last_mouse_y = event.y()
    
    def wheelEvent(self, event):
        """Handle mouse wheel for zooming."""
        delta = event.angleDelta().y() / 120.0
        self.camera.zoom(-delta)
    
    def keyPressEvent(self, event):
        """Handle keyboard input."""
        if event.key() == Qt.Key_Space:
            # Toggle auto-rotation
            self.auto_rotate = not self.auto_rotate
            print(f"Auto-rotation: {'ON' if self.auto_rotate else 'OFF'}")
        
        elif event.key() == Qt.Key_Escape:
            # Exit
            self.close()
    
    def set_audio_reactivity(self, intensity):
        """Set audio reactivity intensity (0.0 to 1.0)."""
        self.current_audio_intensity = intensity
        if self.renderer:
            self.renderer.set_audio_reactivity(intensity)
    
    def cleanup(self):
        """Clean up resources."""
        if self.renderer:
            self.renderer.cleanup()


class ParticleSimulatorApp(QApplication):
    def __init__(self, argv):
        super().__init__(argv)
        
        # Create main widget
        self.widget = ParticleWidget()
        
        # Enable window transparency while keeping title bar
        self.widget.setAttribute(Qt.WA_TranslucentBackground)
        
        # Window configuration - keep normal window with title bar
        self.widget.setWindowFlags(Qt.Window)
        
        # Set window title
        self.widget.setWindowTitle("Particle Simulator")
        
        # Set size
        self.widget.resize(1200, 800)
        
        # Show window
        self.widget.show()
        
        # Start audio server in background thread
        self.audio_server_thread = threading.Thread(target=self.run_audio_server, daemon=True)
        self.audio_server_thread.start()
        
        self.print_welcome()
    
    def run_audio_server(self):
        """Run socket server to receive audio reactivity data."""
        host = 'localhost'
        port = 9999
        
        try:
            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind((host, port))
            server.listen(1)
            
            print(f"\nAudio server listening on {host}:{port}")
            print("   Run 'python audio_capture.py' to send audio data\n")
            
            while True:
                try:
                    client, addr = server.accept()
                    print(f"Audio source connected from {addr}")
                    
                    buffer = ""
                    while True:
                        data = client.recv(1024).decode()
                        if not data:
                            break
                        
                        buffer += data
                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            try:
                                message = json.loads(line)
                                intensity = message.get('intensity', 0.0)
                                self.widget.set_audio_reactivity(intensity)
                            except json.JSONDecodeError:
                                pass
                    
                    print("Audio source disconnected")
                    
                except ConnectionResetError:
                    print("Audio source disconnected")
                except Exception as e:
                    if "WinError 10038" not in str(e):  # Ignore socket closed error
                        print(f"Audio server error: {e}")
        
        except Exception as e:
            print(f"Failed to start audio server: {e}")
    
    def print_welcome(self):
        print("\n" + "═" * 70)
        print("  PARTICLE SIMULATOR")
        print("═" * 70)
        print("\n  CONTROLS:")
        print("  ┌────────────────────────────────────────────────────────────┐")
        print("  │  Left Click + Drag    Rotate camera manually               │")
        print("  │  Mouse Wheel          Zoom in/out                          │")
        print("  │  SPACE                Toggle auto-rotation                 │")
        print("  │  ESC                  Exit application                     │")
        print("  └────────────────────────────────────────────────────────────┘")
        print("  • Audio-reactive curl strength (socket server on port 9999)")
        print("═" * 70 + "\n")


def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(project_root)
    app = ParticleSimulatorApp(sys.argv)
    try:
        sys.exit(app.exec_())
    except KeyboardInterrupt:
        print("\nShutting down...")
        app.widget.cleanup()
if __name__ == '__main__':
    main()