from flask import Flask, Response, render_template
from flask_socketio import SocketIO, emit
import subprocess
import sys

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

def gen_frames():
    """libcamera-still로 MJPEG 스트림"""
    while True:
        # 한 프레임씩 캡처
        result = subprocess.run(
            ['libcamera-still', '-o', '-', '-t', '1', '--width', '640', '--height', '480', '-n'],
            capture_output=True
        )
        
        if result.returncode == 0:
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + result.stdout + b'\r\n')

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
    socketio.run(app, host="0.0.0.0", port=port, debug=False, allow_unsafe_werkzeug=True)

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    start_web_server(port)
