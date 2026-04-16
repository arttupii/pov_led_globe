#!/usr/bin/env python3
# Lähetä kuva ESP32 POV-näytölle UDP-datagrammeina

from PIL import Image, ImageChops
import socket
import sys
import time
import cv2
import os

def deliver(img):
    img = img.convert("RGB").resize((180, 60), Image.Resampling.LANCZOS).transpose(Image.FLIP_TOP_BOTTOM).transpose(Image.Transpose.FLIP_LEFT_RIGHT)

    img = ImageChops.offset(img, -50, 0)   # –dx = vasemmalle

    w, h = img.size
    pix  = img.load()



    row_bytes = 1 + w * 4                       # 1 rivitavu + R,G,B,Brt × w
    packet_bytes_max = row_bytes * ROWS_PER_PACKET
    if packet_bytes_max > 1472:
        sys.exit(f"Pakkaus liian suuri ({packet_bytes_max} B) – "
                "laske ROWS_PER_PACKET arvoa")

    # --- lähetä rivi-/riviparipaketteja ----------------------------------------
    packet = bytearray()
    for y in range(h):
        packet.append(y & 0xFF)                 # rivin tunnustavu
        for x in range(w):
            r, g, b = pix[x, y]
            packet.extend((r, g, b, BRIGHT))

        # nippu täynnä → lähetä
        if (y % ROWS_PER_PACKET) == ROWS_PER_PACKET - 1:
            sock.sendto(packet, (ESP_IP, ESP_PORT))
            packet.clear()

            # try:
            #     data, _ = sock.recvfrom(32)
            #     if data != b"ACK":
            #         print("Odottamaton vastaus:", data)
            # except socket.timeout:
            #     print("Ei vastausta", TIMEOUT_S, "s kuluessa")

    # jos viimeinen jää vajaaksi
    if packet:
        sock.sendto(packet, (ESP_IP, ESP_PORT))

    # signaali “piirto valmis / vaihda bufferi”
    sock.sendto(bytes([0xFF]), (ESP_IP, ESP_PORT))
    # print("img delivered!")
    #debug for deliver rate
    time.sleep(FRAME_DELAY_S)       # optional – tasaiseen päivitykseen


ESP_IP   = "192.168.4.1"
ESP_PORT = 22222

ROWS_PER_PACKET = 2            # montako kuvansiivua / UDP-paketti
BRIGHT          = 5           # kirkkaustavu jokaiselle pikselille
TIMEOUT_S       = 0.010          # vastausodotus sekunteina
FRAME_DELAY_S   = 0.08      # 30 ms = ~33 FPS

# --- UDP-socket ------------------------------------------------------------
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(TIMEOUT_S)

if len(sys.argv) < 2:
    sys.exit(f"käyttö: {sys.argv[0]} kuva.jpg")
img = 1

# --- kuva sisään & skaalaus ------------------------------------------------
if sys.argv[1]=="--pipe":
    raw = sys.stdin.buffer.read()      # ota kaikki tavut putkesta
    img  = Image.open(io.BytesIO(raw)) # "tiedosto-objekti" muistissa

elif sys.argv[1]=="--save":
    packet = bytes([255,2,int(sys.argv[2])])
    sock.sendto(packet, (ESP_IP, ESP_PORT))
    sock.close()
    exit (0)
elif sys.argv[1]=="--delete":
    packet = bytes([255,3,int(sys.argv[2])])
    sock.sendto(packet, (ESP_IP, ESP_PORT))
    sock.close()
    exit (0)

elif len(sys.argv) > 2 and sys.argv[2]=="--video":
    
    cam = cv2.VideoCapture(sys.argv[1]) #käy video läpi kuva kuvalta ja lähetä ne palloon
    TARGET_FPS = 33 
    video_fps = cam.get(cv2.CAP_PROP_FPS)

    frame_skip = max(1, int(video_fps / TARGET_FPS))
    try:

        if not os.path.exists('data'):
            os.makedirs('data')

    except OSError:
        print ('Error: Creating directory of data')
    currentframe = 0
    i = 0
    i = 0

    while True:
        ret, frame = cam.read()
        if not ret:
            break

        if i % frame_skip == 0:
            frame = cv2.resize(frame, (180, 60))
            img = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
            deliver(img)

        i += 1


    sock.close()
    cam.release()
    cv2.destroyAllWindows()
    
    print("video finished")

    #handling images and sending
    # filepaths = []
    # for file in os.listdir("data"):
    #     filepath = os.path.join(os.getcwd(),"data",file)
    #     filepaths.append(filepath)
    # filepaths.sort()
    # for file in filepaths:
    #     img = Image.open(file)
    #     deliver(img)
    



else: 
    img = (Image.open(sys.argv[1]))
    deliver(img)
    sock.close()
