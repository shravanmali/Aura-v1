# Aura v1 face tracking and motor controller
# Author: Shravan Mali
# Tracks faces using DroidCam and sends steering commands to ESP32 over serial

import cv2
import numpy as np
import time
import os
import urllib.request
import urllib.error
import serial
import threading
import sys

class DroidCamStream:
    def __init__(self, url="http://192.168.31.75:4747/video"):
        self.url = url
        self.frame = None
        self.has_frame = False
        self.running = True
        self.lock = threading.Lock()

        if os.path.exists("camera_source.txt"):
            with open("camera_source.txt", "r", encoding="utf-8") as f:
                saved_url = f.read().strip()
                if saved_url:
                    self.url = saved_url

        self.thread = threading.Thread(target=self._read_stream, daemon=True)
        self.thread.start()

    def _read_stream(self):
        while self.running:
            try:
                request = urllib.request.Request(self.url, headers={'User-Agent': 'Mozilla/5.0'})
                stream = urllib.request.urlopen(request, timeout=2.0)
                buffer = b''

                while self.running:
                    chunk = stream.read(4096)
                    if not chunk:
                        break
                    buffer += chunk

                    start = buffer.find(b'\xff\xd8')
                    end = buffer.find(b'\xff\xd9')

                    if start != -1 and end != -1:
                        jpeg_bytes = buffer[start:end+2]
                        buffer = buffer[end+2:]
                        decoded_img = cv2.imdecode(np.frombuffer(jpeg_bytes, dtype=np.uint8), cv2.IMREAD_COLOR)

                        if decoded_img is not None:
                            with self.lock:
                                self.has_frame = True
                                self.frame = cv2.flip(decoded_img, 1)

            except (urllib.error.URLError, OSError, TimeoutError):
                with self.lock:
                    self.has_frame = False
                time.sleep(1.0)

    def get_frame(self):
        with self.lock:
            if self.has_frame and self.frame is not None:
                return True, self.frame.copy()
            return False, None

    def stop(self):
        self.running = False


class RobotSerial:
    def __init__(self, port="COM6", baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.connected = False
        self.last_command = 'S'
        self.last_send_time = 0
        self.lock = threading.Lock()

        self._connect()

    def _connect(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.05)
            self.connected = True
            print(f"[SERIAL] Connected to ESP32 on {self.port} at {self.baud} baud")
        except (serial.SerialException, OSError) as e:
            print(f"[SERIAL WARNING] Could not open {self.port} ({e}). Running in view-only mode.")

    def send(self, cmd):
        now = time.time()
        if cmd == self.last_command and (now - self.last_send_time < 0.05):
            return

        self.last_command = cmd
        self.last_send_time = now

        if self.connected and self.ser and self.ser.is_open:
            try:
                self.ser.write(cmd.encode('ascii'))
            except (serial.SerialException, OSError):
                self.connected = False


def draw_rover_widget(frame, cmd, x=480, y=260):
    cv2.rectangle(frame, (x, y), (x + 130, y + 150), (45, 45, 45), -1)
    cv2.rectangle(frame, (x, y), (x + 130, y + 150), (0, 220, 255), 2)
    cv2.putText(frame, "AURA v1", (x + 25, y + 70), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    cv2.putText(frame, "4WD CHASSIS", (x + 18, y + 90), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 220, 255), 1)

    wheel_coords = [
        ("L", x - 18, y + 10),
        ("L", x - 18, y + 95),
        ("R", x + 130, y + 10),
        ("R", x + 130, y + 95),
    ]

    for side, wx, wy in wheel_coords:
        if cmd == 'F':
            color = (0, 255, 0)
        elif cmd == 'B':
            color = (0, 0, 255)
        elif cmd == 'L':
            color = (0, 0, 255) if side == "L" else (0, 255, 0)
        elif cmd == 'R':
            color = (0, 255, 0) if side == "L" else (0, 0, 255)
        else:
            color = (120, 120, 120)

        cv2.rectangle(frame, (wx, wy), (wx + 18, wy + 45), color, -1)
        cv2.rectangle(frame, (wx, wy), (wx + 18, wy + 45), (255, 255, 255), 1)


def main():
    print("Starting Aura v1 tracking...")

    cam = DroidCamStream()
    robot = RobotSerial(port="COM6")

    cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
    face_detector = cv2.CascadeClassifier(cascade_path)

    win_name = "Aura v1 Dashboard"
    cv2.namedWindow(win_name, cv2.WINDOW_AUTOSIZE)

    LEFT_ZONE = 240
    RIGHT_ZONE = 400

    target_x = 320.0
    target_width = 150.0
    has_target = False
    last_target_seen = 0

    manual_mode = False
    manual_start_time = 0
    current_cmd = 'S'

    fps_last_time = time.time()

    while True:
        now = time.time()
        time_diff = max(0.001, now - fps_last_time)
        fps_last_time = now
        current_fps = int(1.0 / time_diff)

        has_frame, frame = cam.get_frame()

        if not has_frame or frame is None:
            frame = np.zeros((480, 640, 3), dtype=np.uint8)
            cv2.putText(frame, "Waiting for DroidCam...", (30, 220), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 165, 255), 2)
            cv2.putText(frame, f"Open DroidCam on phone at: {cam.url}", (30, 260), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 200), 1)
            cv2.putText(frame, "Press W, A, S, D to test motors manually", (30, 300), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (200, 200, 200), 1)
        else:
            if frame.shape[1] != 640 or frame.shape[0] != 480:
                frame = cv2.resize(frame, (640, 480))

        if manual_mode and (now - manual_start_time > 2.0):
            manual_mode = False

        if not manual_mode and has_frame and frame is not None:
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            faces = face_detector.detectMultiScale(gray, scaleFactor=1.15, minNeighbors=5, minSize=(60, 60))

            if len(faces) > 0:
                has_target = True
                last_target_seen = now

                largest = faces[0]
                for f in faces:
                    if f[2] * f[3] > largest[2] * largest[3]:
                        largest = f

                fx, fy, fw, fh = largest
                center_x = fx + (fw / 2.0)

                target_x = (0.35 * center_x) + (0.65 * target_x)
                target_width = (0.35 * fw) + (0.65 * target_width)

                draw_x = int(target_x - (target_width / 2.0))
                draw_w = int(target_width)

                cv2.rectangle(frame, (draw_x, fy), (draw_x + draw_w, fy + draw_w), (0, 255, 120), 2)
                cv2.circle(frame, (int(target_x), fy + (draw_w // 2)), 5, (0, 0, 255), -1)

                if target_x < LEFT_ZONE:
                    current_cmd = 'L'
                elif target_x > RIGHT_ZONE:
                    current_cmd = 'R'
                else:
                    if target_width < 115:
                        current_cmd = 'F'
                    elif target_width > 230:
                        current_cmd = 'B'
                    else:
                        current_cmd = 'S'

                cv2.putText(frame, "Tracking", (draw_x, max(25, fy - 10)),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 120), 2)

            else:
                if has_target and (now - last_target_seen < 0.4):
                    pass
                else:
                    has_target = False
                    current_cmd = 'S'

        robot.send(current_cmd)

        cv2.line(frame, (LEFT_ZONE, 0), (LEFT_ZONE, 480), (70, 70, 70), 1)
        cv2.line(frame, (RIGHT_ZONE, 0), (RIGHT_ZONE, 480), (70, 70, 70), 1)
        cv2.putText(frame, "ZONE: LEFT", (30, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.45,
                    (0, 220, 255) if current_cmd == 'L' else (150, 150, 150), 1)
        cv2.putText(frame, "ZONE: CENTER", (270, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.45,
                    (0, 255, 0) if current_cmd == 'F' else (150, 150, 150), 1)
        cv2.putText(frame, "ZONE: RIGHT", (470, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.45,
                    (0, 220, 255) if current_cmd == 'R' else (150, 150, 150), 1)

        cv2.rectangle(frame, (0, 0), (640, 28), (20, 20, 20), -1)
        conn_text = "ESP32: CONNECTED (COM6)" if robot.connected else "ESP32: NOT CONNECTED"
        conn_color = (0, 255, 0) if robot.connected else (0, 0, 255)
        cv2.putText(frame, "Aura v1", (15, 19), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        cv2.putText(frame, conn_text, (380, 19), cv2.FONT_HERSHEY_SIMPLEX, 0.45, conn_color, 1)

        cv2.rectangle(frame, (10, 390), (630, 470), (25, 25, 25), -1)
        cv2.rectangle(frame, (10, 390), (630, 470), (60, 60, 60), 1)

        motion_text = "STOPPED"
        motion_color = (200, 200, 200)
        if current_cmd == 'F':
            motion_text = "FORWARD"
            motion_color = (0, 255, 0)
        elif current_cmd == 'B':
            motion_text = "BACKWARD"
            motion_color = (0, 0, 255)
        elif current_cmd == 'L':
            motion_text = "PIVOT LEFT"
            motion_color = (0, 215, 255)
        elif current_cmd == 'R':
            motion_text = "PIVOT RIGHT"
            motion_color = (0, 215, 255)

        cv2.putText(frame, f"MOTOR: {motion_text}", (25, 422), cv2.FONT_HERSHEY_SIMPLEX, 0.6, motion_color, 2)
        mode_label = "MANUAL OVERRIDE (WASD)" if manual_mode else ("AUTO TRACKING" if has_target else "WAITING FOR TARGET")
        cv2.putText(frame, f"MODE: {mode_label} | FPS: {current_fps}", (25, 452), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (180, 180, 180), 1)

        draw_rover_widget(frame, current_cmd, x=480, y=260)

        cv2.imshow(win_name, frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord(' '):
            manual_mode = True
            manual_start_time = now
            current_cmd = 'S'
            robot.send('S')
        elif key in [ord('w'), ord('W')]:
            manual_mode = True
            manual_start_time = now
            current_cmd = 'F'
            robot.send('F')
        elif key in [ord('s'), ord('S')]:
            manual_mode = True
            manual_start_time = now
            current_cmd = 'B'
            robot.send('B')
        elif key in [ord('a'), ord('A')]:
            manual_mode = True
            manual_start_time = now
            current_cmd = 'L'
            robot.send('L')
        elif key in [ord('d'), ord('D')]:
            manual_mode = True
            manual_start_time = now
            current_cmd = 'R'
            robot.send('R')

    robot.send('S')
    cam.stop()
    cv2.destroyAllWindows()
    print("Dashboard closed safely.")

if __name__ == "__main__":
    main()
