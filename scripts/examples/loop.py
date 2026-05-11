#!/usr/bin/python3
import json
import requests
import time
from typing import Any

ESP32_IP = "192.168.1.228"
BASE_URL = f"http://{ESP32_IP}"

def post(path, payload):
    url = BASE_URL + path
    try:
        with requests.post(url, json=payload, timeout=(10,10), headers={"Connection": "close"}) as response:
            # print("status:", response.status_code)
            # print("text:", response.text)
            print(f"Time taken: {response.elapsed.total_seconds()} seconds")
            response.raise_for_status()
            return response
    except Exception as e:
        print(f"ERROR: {e}")
    except:
        pass


def main():
    screen = 0

    while True:
        time.sleep(30)
        screen = (screen + 1) % 6
        post("/api/showScreen", {"screen": screen})


if __name__ == "__main__":
    main()
