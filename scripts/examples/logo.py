import json
import requests
import time
from typing import Any

ESP32_IP = "192.168.1.228"
BASE_URL = f"http://{ESP32_IP}"

def post(path, payload):
    url = BASE_URL + path
    # print("POST", url)
    # print("payload:", payload)
    try:
        with requests.post(url, json=payload, timeout=(10,10), headers={"Connection": "close"}) as response:
            # print("status:", response.status_code)
            # print("text:", response.text)
            print(f"Time taken: {response.elapsed.total_seconds()} seconds")
            response.raise_for_status()
            return response
    except Exception as e:
        print(f"ERROR: {e}")

def clear_screen(screen):
    post("/api/clearScreen", {"screen": screen})

def show_screen(screen):
    post("/api/showScreen", {"screen": screen})

def set_text(screen, slot, x, y, text, r, g, b, size=1, visible=True):
    post("/api/object",
        make_text(screen, slot, x, y, text, r, g, b, size, visible)
    )

def make_text(screen, slot, x, y, text, r, g, b, size=1, visible=True):
    return {
        "screen": screen,
        "slot": slot,
        "object": {
            "type": "text",
            "visible": visible,
            "x": x,
            "y": y,
            "r": r,
            "g": g,
            "b": b,
            "text": text,
            "size": size,
        },
    }

def set_rect(screen, slot, x, y, w, h, r, g, b, filled=False, visible=True):
    post("/api/object",
        make_rect(screen, slot, x, y, w, h, r, g, b, filled, visible)
    )

def make_rect(screen, slot, x, y, w, h, r, g, b, filled=False, visible=True):
    return {
        "screen": screen,
        "slot": slot,
        "object": {
            "type": "rect",
            "visible": visible,
            "x": x,
            "y": y,
            "w": w,
            "h": h,
            "filled": filled,
            "r": r,
            "g": g,
            "b": b,
        },
    }

def set_circle(screen, slot, x, y, radius, r, g, b, filled=False, visible=True):
    post("/api/object",
        make_circle(screen, slot, x, y, radius, r, g, b, filled, visible)
    )

def make_circle(screen, slot, x, y, radius, r, g, b, filled=False, visible=True):
    return {
        "screen": screen,
        "slot": slot,
        "object": {
            "type": "circle",
            "visible": visible,
            "x": x,
            "y": y,
            "radius": radius,
            "filled": filled,
            "r": r,
            "g": g,
            "b": b,
        },
    }

def set_line(screen, slot, x1, y1, x2, y2, r, g, b, visible=True):
    post("/api/object",
        make_line(screen, slot, x1, y1, x2, y2, r, g, b, visible)
    )

def make_line(screen, slot, x1, y1, x2, y2, r, g, b, visible=True):
    return {
        "screen": screen,
        "slot": slot,
        "object": {
            "type": "line",
            "visible": visible,
            "x1": x1,
            "y1": y1,
            "x2": x2,
            "y2": y2,
            "r": r,
            "g": g,
            "b": b,
        },
    }

def set_batch(payloads: list[dict[str, Any]]):
    post("/api/object", {"objects": payloads})
    

def main():
    screen = 0

    clear_screen(screen)
    show_screen(screen)

    set_batch([
#        make_rect(screen, 0, 0, 0, 64, 32, 255, 0, 255, filled=False),
        make_line(screen, 1, 0, 0, 63, 31, 255, 0, 0),
        make_circle(screen, 2, 32, 14, 12, 0, 0, 255, filled=True),
        make_circle(screen, 3, 38, 10, 3, 192, 192, 192, filled=True),
        make_text(screen, 4, 4, 22, "NASA", 162, 255, 162, size=1),
    ])

    while True:
        time.sleep(60)
        screen = (screen + 1) % 6
        show_screen(screen)


if __name__ == "__main__":
    main()
