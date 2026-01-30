#!/usr/bin/env python3
"""
Audio Output Capture for Particle Simulator
Captures system audio output (what you hear) from Windows and sends
the audio intensity to the particle simulator in real-time.
"""

import sys
import socket
import time
import numpy as np
import sounddevice as sd
import threading
import json

SAMPLE_RATE = 44100
BLOCK_SIZE = 1024  # Smaller for lower latency
SOCKET_HOST = 'localhost'
SOCKET_PORT = 9999
MIN_RMS = 0.001  # Noise floor
MAX_RMS = 0.3    # Maximum expected RMS (adjust based on your system)
SMOOTHING = 0.7  # Smoothing factor (0.0 = no smoothing, 1.0 = max smoothing)


class AudioCapture:
    def __init__(self):
        self.running = False
        self.socket = None
        self.current_intensity = 0.0
        self.last_intensity = 0.0
        
    def find_output_device(self):
        devices = sd.query_devices()
        
        # On Windows, look for "Stereo Mix" or loopback devices
        # Common names: "Stereo Mix", "Wave Out Mix", "What U Hear"
        loopback_keywords = ['stereo mix', 'wave out', 'what u hear', 'loopback']
        
        print("\nAvailable audio devices:")
        print("-" * 70)
        for i, device in enumerate(devices):
            max_inputs = device.get('max_input_channels', 0)
            if max_inputs > 0:
                print(f"[{i}] {device['name']} (inputs: {max_inputs})")
        
        # Try to find loopback device
        for i, device in enumerate(devices):
            name = device['name'].lower()
            max_inputs = device.get('max_input_channels', 0)
            if max_inputs > 0:
                for keyword in loopback_keywords:
                    if keyword in name:
                        print(f"\nFound loopback device: {device['name']}")
                        return i
        
        print("\nNo loopback device found automatically.")
        print("On Windows, you may need to enable 'Stereo Mix' in Sound settings:")
        print("  1. Right-click speaker icon → Sounds → Recording tab")
        print("  2. Right-click empty space → Show Disabled Devices")
        print("  3. Enable 'Stereo Mix' and set as default")
        print("\nEnter device number manually (or press Enter for default): ", end='')
        
        user_input = input().strip()
        if user_input:
            return int(user_input)
        return sd.default.device[0]  # Default input device
    
    def connect_to_simulator(self):
        """Connect to the particle simulator via socket."""
        max_retries = 30
        retry_delay = 1.0
        
        for attempt in range(max_retries):
            try:
                self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.socket.connect((SOCKET_HOST, SOCKET_PORT))
                print(f"Connected to particle simulator on {SOCKET_HOST}:{SOCKET_PORT}")
                return True
            except ConnectionRefusedError:
                if attempt == 0:
                    print(f"Waiting for particle simulator to start on {SOCKET_HOST}:{SOCKET_PORT}...")
                time.sleep(retry_delay)
            except Exception as e:
                print(f"Connection error: {e}")
                return False
        
        print("Failed to connect to particle simulator.")
        print("Make sure the particle simulator is running!")
        return False
    
    def send_intensity(self, intensity):
        """Send audio intensity to particle simulator."""
        if self.socket:
            try:
                data = json.dumps({'intensity': float(intensity)}) + '\n'
                self.socket.sendall(data.encode())
            except (BrokenPipeError, ConnectionResetError):
                print("Connection to simulator lost")
                self.running = False
    
    def audio_callback(self, indata, frames, time_info, status):
        """Called by sounddevice for each audio block."""
        if status:
            print(f"Audio status: {status}")
        
        # Calculate RMS (root mean square) - measure of audio intensity
        audio_data = indata[:, 0] if indata.ndim > 1 else indata
        rms = np.sqrt(np.mean(audio_data**2))
        
        # Normalize to 0.0 - 1.0 range
        normalized = (rms - MIN_RMS) / (MAX_RMS - MIN_RMS)
        normalized = np.clip(normalized, 0.0, 1.0)
        
        # Apply smoothing to reduce jitter
        self.current_intensity = (SMOOTHING * self.last_intensity + 
                                 (1 - SMOOTHING) * normalized)
        self.last_intensity = self.current_intensity
        
        # Send to simulator
        self.send_intensity(self.current_intensity)
    
    def run(self):
        """Main capture loop."""
        print("\n" + "═" * 70)
        print("  AUDIO CAPTURE - System Output Monitor")
        print("═" * 70)
        
        # Find audio device
        device_id = self.find_output_device()
        
        # Connect to simulator
        if not self.connect_to_simulator():
            return
        
        print("\n" + "═" * 70)
        print("  CAPTURING AUDIO")
        print("═" * 70)
        print("\nPlay audio on your PC")
        
        self.running = True
        
        try:
            with sd.InputStream(
                device=device_id,
                channels=1,
                samplerate=SAMPLE_RATE,
                blocksize=BLOCK_SIZE,
                callback=self.audio_callback
            ):
                while self.running:
                    bar_length = 50
                    filled = int(self.current_intensity * bar_length)
                    bar = '█' * filled + '░' * (bar_length - filled)
                    print(f"\rIntensity: [{bar}] {self.current_intensity:.2f}", end='', flush=True)
                    time.sleep(0.05)
        
        except KeyboardInterrupt:
            print("\n\nStopping audio capture...")
        except Exception as e:
            print(f"\n\nError: {e}")
        finally:
            if self.socket:
                self.socket.close()
            print("✓ Audio capture stopped")


def main():
    """Entry point."""
    capture = AudioCapture()
    capture.run()


if __name__ == '__main__':
    main()
