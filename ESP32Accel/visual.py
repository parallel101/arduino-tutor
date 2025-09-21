import pygame
from pyquaternion import Quaternion
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import numpy as np
from threading import Thread
from queue import Queue, Full, Empty
import serial
import time
import os
import re

MAG_CALIB = '''
-71.790718 -52.549339 18.06226 10.764116 41.272274 104.961983
'''

class OrientationVisualizer:
    WANTED_KEYS = {'t', 'ax', 'ay', 'az', 'gx', 'gy', 'gz', 'mx', 'my', 'mz'}
    WIN_SIZE = (1920, 1440)
    WIN_TITLE = 'ESP32 Orientation'

    def __init__(self):
        os.environ['SDL_VIDEO_CENTERED'] = '1'
        pygame.init()
        pygame.display.set_mode(self.WIN_SIZE, DOUBLEBUF | OPENGL)
        pygame.display.set_caption(self.WIN_TITLE)
        glutInit()

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(40.0, (self.WIN_SIZE[0] / self.WIN_SIZE[1]), 0.05, 200.0)
        glMatrixMode(GL_MODELVIEW)
        gluLookAt(1.0, -2.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING)
        glEnable(GL_COLOR_MATERIAL)

        glEnable(GL_LIGHT0)
        glLightfv(GL_LIGHT0, GL_SPECULAR, [1, 1, 1])
        glLightfv(GL_LIGHT0, GL_AMBIENT, [0.1, 0.1, 0.1])
        glLightfv(GL_LIGHT0, GL_DIFFUSE, [1, 1, 1])
        glLightfv(GL_LIGHT0, GL_POSITION, [-2, -3, 4, 0])
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, [1, 1, 1])
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, 128)

        self.data_queue = Queue()
        self.current_data = None
        self.last_magnet = None
        self.accumulated_magnet = None
        self.running = True

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
                if all(key in row for key in self.WANTED_KEYS):
                    self.put_data(row)

        except KeyboardInterrupt:
            pass
        finally:
            com.close()

    def put_data(self, data):
        try:
            self.data_queue.put_nowait(data)
        except Full:
            pass

    def get_latest_data(self):
        while not self.data_queue.empty():
            try:
                self.current_data = self.data_queue.get_nowait()
            except Empty:
                pass
        return self.current_data

    def get_magnet_vector(self, data):
        m = np.array([data['mx'], data['my'], data['mz']])
        return m

    def get_filtered_magnet_vector(self, data):
        m = self.get_magnet_vector(data)
        if self.last_magnet is not None:
            delta_magnet = m - self.last_magnet
            delta_magnet /= (np.linalg.norm(self.last_magnet + m) / 2) or 1
            assert self.accumulated_magnet is not None
            self.accumulated_magnet += delta_magnet
        else:
            self.accumulated_magnet = m

        norm_magnet = np.linalg.norm(self.accumulated_magnet)
        if norm_magnet == 0:
            self.accumulated_magnet = m
        self.accumulated_magnet /= norm_magnet or 1
        self.accumulated_magnet[np.isnan(self.accumulated_magnet) | np.isinf(self.accumulated_magnet)] = 0

        self.last_magnet = m
        return self.accumulated_magnet * np.linalg.norm(m)

    def get_accel_vector(self, data):
        a = np.array([data['ax'], data['ay'], data['az']])
        return a

    def get_gyro_vector(self, data):
        g = np.array([data['gx'], data['gy'], data['gz']])
        return g

    def compute_rotation(self, accel, magnet):
        Z = np.array(accel)
        Y = np.array(magnet)

        Z /= np.linalg.norm(Z) or 1
        # Y -= np.dot(Y, Z) * Z
        Y /= np.linalg.norm(Y) or 1

        X = np.cross(Y, Z)
        X /= np.linalg.norm(X) or 1
        Y = np.cross(Z, X)

        R = np.array([X, Y, Z])
        return Quaternion(matrix=R)

    def draw_arrow(self, vector):
        r = np.linalg.norm(vector)

        ax = np.array([1.0, 0.0, 0.0])
        az = vector
        az /= np.linalg.norm(vector) or 1
        ay = np.cross(az, ax)
        ay /= np.linalg.norm(ay) or 1
        ax = np.cross(ay, az)

        matrix = np.eye(4)
        matrix[:3, :3] = np.array([ax, ay, az])

        quadric = gluNewQuadric()
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glMultMatrixd(matrix)
        gluCylinder(quadric, 0.02, 0.02, r * 0.9, 10, 10)
        glTranslatef(0, 0, r * 0.9)
        gluCylinder(quadric, 0.05, 0.0, r * 0.1, 10, 10)
        glPopMatrix()
        gluDeleteQuadric(quadric)

    def draw_teapot(self, matrix, size):
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glMultMatrixd(np.array(matrix).astype(np.float64))
        glRotatef(90, 1, 0, 0)
        glutSolidTeapot(size)
        glPopMatrix()
    
    def draw_axes(self):
        glDisable(GL_LIGHTING)
        glBegin(GL_LINES)
        glColor3f(1.0, 0.0, 0.0)
        glVertex3f(0, 0, 0)
        glVertex3f(1, 0, 0)
        glColor3f(0.0, 1.0, 0.0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 1, 0)
        glColor3f(0.0, 0.0, 1.0)
        glVertex3f(0, 0, 0)
        glVertex3f(0, 0, 1)
        glEnd()

        self.render_text("X", 1.1, 0, 0)
        self.render_text("Y", 0, 1.1, 0)
        self.render_text("Z", 0, 0, 1.1)
        glEnable(GL_LIGHTING)

    def render_text(self, text, x, y, z):
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        glLoadIdentity()
        glMatrixMode(GL_PROJECTION)
        glPushMatrix()
        glLoadIdentity()
        glDisable(GL_LIGHTING)
        glDisable(GL_DEPTH_TEST)
        font = pygame.font.SysFont('Arial', 24)
        text_surface = font.render(text, True, (255, 255, 255, 255), (0, 0, 0, 0))
        text_data = pygame.image.tostring(text_surface, "RGBA", True)

        glRasterPos3d(x, y, z)
        glDrawPixels(text_surface.get_width(), text_surface.get_height(), GL_RGBA, GL_UNSIGNED_BYTE, text_data)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING)
        glMatrixMode(GL_PROJECTION)
        glPopMatrix()
        glMatrixMode(GL_MODELVIEW)
        glPopMatrix()

    def run(self):
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                    self.running = False
                    pygame.quit()
                    return

            pressed = pygame.key.get_pressed()
            if pressed[pygame.K_a]:
                glTranslatef(0.02, 0, 0)
            if pressed[pygame.K_d]:
                glTranslatef(-0.02, 0, 0)
            if pressed[pygame.K_w]:
                glTranslatef(0, -0.02, 0)
            if pressed[pygame.K_s]:
                glTranslatef(0, 0.02, 0)
            if pressed[pygame.K_q]:
                glTranslatef(0, 0, -0.02)
            if pressed[pygame.K_e]:
                glTranslatef(0, 0, 0.02)

            if pygame.mouse.get_pressed()[0]:
                rel = pygame.mouse.get_rel()
                glRotatef(rel[0] * 0.5, 0, 0, 1)
                glRotatef(rel[1] * 0.5, 1, 0, 0)
            else:
                pygame.mouse.get_rel()

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) # type: ignore

            data = self.get_latest_data()
            if data:
                magnet = self.get_magnet_vector(data)
                accel = self.get_accel_vector(data)
                gyro = self.get_gyro_vector(data)
                rotation = self.compute_rotation(accel, magnet)

                magnet = rotation.rotation_matrix.T @ magnet
                accel = rotation.rotation_matrix.T @ accel
                gyro = rotation.rotation_matrix.T @ gyro

                self.draw_axes()
                glColor3f(1.0, 0.75, 0.5)
                self.draw_teapot(rotation.transformation_matrix.T, 0.5)
                glColor3f(0.8, 0.4, 0.9)
                self.draw_arrow(magnet / 30.0)
                glColor3f(1.0, 0.5, 0.0)
                self.draw_arrow(accel / 10.0)
                glColor3f(0.2, 0.9, 0.0)
                self.draw_arrow(gyro / 1.0)

                glColor3f(1.0, 1.0, 1.0)
                self.render_text(f"t: {data['t']:.3f}", -0.75, 0.8, 0)
                self.render_text(f"ax: {data['ax']:.3f}", -0.75, 0.75, 0)
                self.render_text(f"ay: {data['ay']:.3f}", -0.75, 0.7, 0)
                self.render_text(f"az: {data['az']:.3f}", -0.75, 0.65, 0)
                self.render_text(f"mx: {data['mx']:.3f}", -0.75, 0.6, 0)
                self.render_text(f"my: {data['my']:.3f}", -0.75, 0.55, 0)
                self.render_text(f"mz: {data['mz']:.3f}", -0.75, 0.5, 0)
                self.render_text(f"gx: {data['gx']:.3f}", -0.75, 0.45, 0)
                self.render_text(f"gy: {data['gy']:.3f}", -0.75, 0.4, 0)
                self.render_text(f"gz: {data['gz']:.3f}", -0.75, 0.35, 0)

            pygame.display.flip()
            pygame.time.wait(10)

    def calibrate_magnet(self):
        minx, miny, minz = 999.0, 999.0, 999.0
        maxx, maxy, maxz = -999.0, -999.0, -999.0

        while True:
            data = self.get_latest_data()
            if data:
                minx = min(minx, data['mx'])
                miny = min(miny, data['my'])
                minz = min(minz, data['mz'])
                maxx = max(maxx, data['mx'])
                maxy = max(maxy, data['my'])
                maxz = max(maxz, data['mz'])
                print(minx, miny, minz, maxx, maxy, maxz)

            time.sleep(0.05)

if __name__ == "__main__":
    visualizer = OrientationVisualizer()
    visualizer.run()
