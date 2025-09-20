import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
from threading import Thread
from queue import Queue, Full, Empty
import serial
import math
import time
import re

class VectorVisualizer:
    def __init__(self):
        # Initialize pygame and OpenGL
        pygame.init()
        display = (800, 600)
        pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
        gluPerspective(45, (display[0]/display[1]), 0.1, 50.0)
        glTranslatef(0.0, 0.0, -5)
        
        # Vector data queue (thread-safe)
        self.data_queue = Queue()
        self.current_vector = np.array([0.0, 0.0, 0.0])
        self.running = True
        
        # Start data update thread
        self.data_thread = Thread(target=self.fetch_data_stream)
        self.data_thread.daemon = True
        self.data_thread.start()
    
    def fetch_data_stream(self):
        com = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
        pattern = re.compile(r'([a-zA-Z]+)=(-?\d*\.\d+)')

        try:
            while self.running:
                line = com.readline().decode().strip()
                if not line:
                    time.sleep(0.1)
                    continue

                matches = pattern.findall(line)
                row = {key: float(value) for key, value in matches}

                if 'mx' in row and 'my' in row and 'mz' in row:
                    vector = np.array([row['mx'], row['my'], row['mz']])
                    # vector[0] += 46.5
                    # vector[1] += 11.5
                    # vector[2] -= 26
                    self.put_data(vector)

        except KeyboardInterrupt:
            pass
        finally:
            com.close()

    def put_data(self, vector):
        try:
            self.data_queue.put_nowait(vector)
        except Full:
            pass
    
    def get_latest_data(self):
        """Get the latest vector data from the queue"""
        while not self.data_queue.empty():
            try:
                self.current_vector = self.data_queue.get_nowait()
            except Empty:
                pass
        return self.current_vector
    
    def draw_arrow(self, vector, color=(1.0, 0.5, 1.0)):
        """Draw a 3D arrow representing the vector"""
        glColor3f(*color)

        r = np.linalg.norm(vector) * 0.1
        rz = math.degrees(math.atan2(-vector[0], vector[1]))
        rx = math.degrees(math.atan2(vector[2], math.hypot(vector[0], vector[1]))) + 90
        
        # Arrow shaft (cylinder)
        glPushMatrix()
        quadric = gluNewQuadric()
        glRotatef(rz, 0, 0, 1)
        glRotatef(rx, 1, 0, 0)
        gluCylinder(quadric, 0.02, 0.02, r * 0.9, 10, 10)
        glPopMatrix()
        
        # Arrow head (cone)
        glPushMatrix()
        glRotatef(rz, 0, 0, 1)
        glRotatef(rx, 1, 0, 0)
        glTranslatef(0, 0, r * 0.9)
        gluCylinder(quadric, 0.05, 0.0, r * 0.1, 10, 10)
        glPopMatrix()
        
        gluDeleteQuadric(quadric)
    
    def draw_axes(self):
        """Draw reference axes"""
        glBegin(GL_LINES)
        # X axis (red)
        glColor3f(1.0, 0.0, 0.0)
        glVertex3f(0, 0, 0)
        glVertex3f(1, 0, 0)
        # Y axis (green)
        glColor3f(0.0, 1.0, 0.0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 1, 0)
        # Z axis (blue)
        glColor3f(0.0, 0.0, 1.0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 1)
        glEnd()
        
        # Add labels
        self.render_text("X", 1.1, 0, 0)
        self.render_text("Y", 0, 1.1, 0)
        self.render_text("Z", 0, 0, 1.1)
    
    def render_text(self, text, x, y, z):
        """Render text at 3D position"""
        font = pygame.font.SysFont('Arial', 20)
        text_surface = font.render(text, True, (255, 255, 255, 255), (0, 0, 0, 0))
        text_data = pygame.image.tostring(text_surface, "RGBA", True)
        
        glRasterPos3d(x, y, z)
        glDrawPixels(text_surface.get_width(), text_surface.get_height(), 
                    GL_RGBA, GL_UNSIGNED_BYTE, text_data)
    
    def run(self):
        """Main rendering loop"""
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False
                    pygame.quit()
                    return
            
            # Handle rotation with mouse
            if pygame.mouse.get_pressed()[0]:
                rel = pygame.mouse.get_rel()
                glRotatef(rel[0], 0, 1, 0)
                glRotatef(rel[1], 1, 0, 0)
            else:
                pygame.mouse.get_rel()  # Reset relative motion
            
            # Clear screen
            glClear(GL_COLOR_BUFFER_BIT)
            
            # Get latest vector data
            vector = self.get_latest_data()
            
            # Draw reference axes
            self.draw_axes()
            
            # Draw the current vector
            self.draw_arrow(vector)
            
            # Display vector values
            glColor3f(1.0, 1.0, 1.0)
            self.render_text(f"X: {vector[0]:.3f}", -1.5, 1.5, 0)
            self.render_text(f"Y: {vector[1]:.3f}", -1.5, 1.3, 0)
            self.render_text(f"Z: {vector[2]:.3f}", -1.5, 1.1, 0)
            self.render_text(f"Magnitude: {np.linalg.norm(vector):.3f}", -1.5, 0.9, 0)
            
            pygame.display.flip()
            pygame.time.wait(10)

if __name__ == "__main__":
    visualizer = VectorVisualizer()
    visualizer.run()
