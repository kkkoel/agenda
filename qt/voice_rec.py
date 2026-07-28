#!/usr/bin/env python3
import vosk
import wave
import json
import sys
import time

print("Loading model...", file=sys.stderr)

try:
    model = vosk.Model('/home/code/Desktop/MySchedule/qt/vosk-model')
    print("Model loaded", file=sys.stderr)
except Exception as e:
    print(f"Model load error: {e}", file=sys.stderr)
    sys.exit(1)

# 等待音频文件准备好
time.sleep(0.5)

try:
    wf = wave.open('/tmp/voice_input.wav', 'rb')
    rec = vosk.KaldiRecognizer(model, wf.getframerate())
    result = ""

    while True:
        data = wf.readframes(4000)
        if len(data) == 0:
            break
        if rec.AcceptWaveform(data):
            res = json.loads(rec.Result())
            if res.get('text'):
                result = res.get('text')

    if not result:
        res = json.loads(rec.FinalResult())
        result = res.get('text', '')

    print(result)
except Exception as e:
    print("", file=sys.stdout)
