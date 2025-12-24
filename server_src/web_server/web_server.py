from flask import Flask, Response, render_template
from picamera2 import Picamera2
import cv2
import time
import sys
import signal

app = Flask(__name__)

# Picamera2 초기화
print("[Web] Initializing camera...")
picam2 = Picamera2()
config = picam2.create_video_configuration(main={"size": (640, 480), "format": "RGB888"})
picam2.configure(config)
picam2.start()
time.sleep(1)
print("[Web] Camera initialized successfully")

def gen_frames():
    """카메라 프레임 생성"""
    while True:
        try:
            # RGB 프레임 캡처
            frame = picam2.capture_array()
            
            # JPEG 인코딩
            ok, buffer = cv2.imencode(".jpg", frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
            if not ok:
                continue
            
            yield (b"--frame\r\n"
                   b"Content-Type: image/jpeg\r\n\r\n" + buffer.tobytes() + b"\r\n")
        except Exception as e:
            print(f"[Web] Frame error: {e}")
            break

@app.route("/")
def index():
    """메인 페이지"""
    return render_template('index.html')

@app.route("/video")
def video():
    """비디오 스트림"""
    return Response(gen_frames(), mimetype="multipart/x-mixed-replace; boundary=frame")

def cleanup():
    """종료 시 카메라 해제"""
    print("[Web] Cleaning up...")
    try:
        picam2.stop()
        print("[Web] Camera stopped")
    except:
        pass

def signal_handler(sig, frame):
    """시그널 핸들러"""
    print("\n[Web] Shutting down...")
    cleanup()
    sys.exit(0)

if __name__ == "__main__":
    # 시그널 핸들러 등록
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
        print(f"[Web] Starting camera server on port {port}")
        app.run(host="0.0.0.0", port=port, debug=False, threaded=True)
    except Exception as e:
        print(f"[Web] Error: {e}")
    finally:
        cleanup()
