from flask import Flask, Response, render_template
from flask_socketio import SocketIO, emit
import subprocess
import threading
import sys
import time

app = Flask(__name__)
app.config['SECRET_KEY'] = 'iot_device_secret'
socketio = SocketIO(app, cors_allowed_origins="*")

device_state = {
    'led_on': False,
    'led_brightness': 1,
    'buzzer_playing': False,
    'light_sensor_value': 0,
    'segment_value': 0,
    'sensor_monitoring': False,
    'segment_counting': False
}

# 카메라 프로세스
camera_process = None

def start_camera_stream():
    """백그라운드에서 카메라 스트림 시작"""
    global camera_process
    
    # rpicam-vid 또는 libcamera-vid 사용 (OS 버전에 따라)
    # Bullseye 이상: rpicam-vid
    # 이전 버전: libcamera-vid
    
    cmd = [
        'rpicam-vid',  # 또는 'libcamera-vid'
        '-t', '0',  # 무한 실행
        '--width', '640',
        '--height', '480',
        '--framerate', '30',
        '--codec', 'mjpeg',
        '-o', '-',  # stdout으로 출력
        '--nopreview',
        '--timeout', '0'
    ]
    
    try:
        camera_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            bufsize=10**8
        )
        print("[Web] Camera stream started")
    except FileNotFoundError:
        # rpicam-vid가 없으면 libcamera-vid 시도
        cmd[0] = 'libcamera-vid'
        camera_process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            bufsize=10**8
        )
        print("[Web] Camera stream started (libcamera-vid)")

def gen_frames():
    """카메라 프로세스에서 프레임 읽기"""
    global camera_process
    
    if camera_process is None:
        return
    
    while True:
        try:
            # JPEG 경계 찾기
            chunk = camera_process.stdout.read(4096)
            if not chunk:
                break
            
            yield chunk
            
        except Exception as e:
            print(f"[Web] Frame error: {e}")
            break

@app.route("/")
def index():
    return render_template('index.html')

@app.route("/video")
def video():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@socketio.on('connect')
def handle_connect():
    print('[Web] Client connected')
    emit('state_update', device_state)

@socketio.on('disconnect')
def handle_disconnect():
    print('[Web] Client disconnected')

def update_device_state(state_dict):
    global device_state
    device_state.update(state_dict)
    socketio.emit('state_update', device_state, broadcast=True)

def start_web_server(port=8000):
    print(f"[Web] Starting web server on port {port}")
    
    # 카메라 스트림 시작
    start_camera_stream()
    time.sleep(2)  # 카메라 초기화 대기
    
    try:
        socketio.run(app, host="0.0.0.0", port=port, debug=False, allow_unsafe_werkzeug=True)
    finally:
        cleanup()

def cleanup():
    global camera_process
    if camera_process:
        camera_process.terminate()
        camera_process.wait()
        print("[Web] Camera stopped")

if __name__ == "__main__":
    try:
        port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
        start_web_server(port)
    except KeyboardInterrupt:
        print("\n[Web] Shutting down...")
        cleanup()
