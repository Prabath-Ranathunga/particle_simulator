"""
Camera utilities for 3D view control.
Handles projection and view matrices.
"""
import numpy as np


class Camera:
    def __init__(self):
        self.position = np.array([0.0, 5.0, 15.0], dtype=np.float32)
        self.target = np.array([0.0, 0.0, 0.0], dtype=np.float32)
        self.up = np.array([0.0, 1.0, 0.0], dtype=np.float32)
        
        self.fov = 45.0
        self.near = 0.1
        self.far = 100.0
        
        # Spherical coordinates for rotation
        self.distance = 15.0
        self.azimuth = 0.0
        self.elevation = 0.3
    
    def set_position(self, x, y, z):
        """Set camera position."""
        self.position = np.array([x, y, z], dtype=np.float32)
    
    def look_at(self, x, y, z):
        """Set camera target."""
        self.target = np.array([x, y, z], dtype=np.float32)
    
    def rotate(self, delta_azimuth, delta_elevation):
        """Rotate camera around target."""
        self.azimuth += delta_azimuth
        self.elevation = np.clip(self.elevation + delta_elevation, -np.pi/2 + 0.1, np.pi/2 - 0.1)
        
        # Calculate new position
        x = self.target[0] + self.distance * np.cos(self.elevation) * np.sin(self.azimuth)
        y = self.target[1] + self.distance * np.sin(self.elevation)
        z = self.target[2] + self.distance * np.cos(self.elevation) * np.cos(self.azimuth)
        
        self.position = np.array([x, y, z], dtype=np.float32)
    
    def zoom(self, delta):
        """Zoom camera in/out."""
        self.distance = np.clip(self.distance + delta, 5.0, 30.0)
        
        # Update position maintaining angle
        direction = self.position - self.target
        direction = direction / np.linalg.norm(direction) * self.distance
        self.position = self.target + direction
    
    def get_view_matrix(self):
        """Calculate view matrix."""
        # Forward vector
        forward = self.target - self.position
        forward = forward / np.linalg.norm(forward)
        
        # Right vector
        right = np.cross(forward, self.up)
        right = right / np.linalg.norm(right)
        
        # Recalculate up vector
        up = np.cross(right, forward)
        
        # View matrix
        view = np.eye(4, dtype=np.float32)
        view[0, :3] = right
        view[1, :3] = up
        view[2, :3] = -forward
        view[0, 3] = -np.dot(right, self.position)
        view[1, 3] = -np.dot(up, self.position)
        view[2, 3] = np.dot(forward, self.position)
        
        return view
    
    def get_projection_matrix(self, aspect_ratio):
        """Calculate perspective projection matrix (OpenGL style)."""
        fov_rad = np.radians(self.fov)
        f = 1.0 / np.tan(fov_rad / 2.0)
        
        projection = np.zeros((4, 4), dtype=np.float32)
        projection[0, 0] = f / aspect_ratio
        projection[1, 1] = f
        projection[2, 2] = (self.far + self.near) / (self.near - self.far)
        projection[3, 2] = -1.0  # Row 3, col 2 (not row 2, col 3)
        projection[2, 3] = (2.0 * self.far * self.near) / (self.near - self.far)
        
        return projection