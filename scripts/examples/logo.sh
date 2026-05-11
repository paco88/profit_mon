#!/bin/bash

# batch

curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"objects": [
{"screen":0, "slot":0, "object":{"type":"circle", "visible":true, "x":42, "y":18, "radius": 12, "filled": true, "r":0, "b":255, "g":0}},
{"screen":0, "slot":1, "object":{"type":"rect", "visible":true, "x":46, "y":4, "w": 8, "h": 8, "filled": true, "r":0, "b":0, "g":128}},
{"screen":0, "slot":2, "object":{"type":"text", "visible":true, "x":1, "y":4, "r":255, "b":255, "g":0, "text":"Be", "size":1}},
{"screen":0, "slot":3, "object":{"type":"text", "visible":true, "x":9, "y":12, "r":0, "b":255, "g":255, "text":"Your", "size":1}},
{"screen":0, "slot":4, "object":{"type":"text", "visible":true, "x":5, "y":20, "r":255, "b":0, "g":255, "text":"Self", "size":1}},
{"screen":0, "slot":5, "object":{"type":"line", "visible":true, "x1":38, "y1":32, "x2":64, "y2":6, "r":128, "b":128, "g":128}},
{"screen":1, "slot":0, "object":{"type":"text", "visible":true, "x":9, "y":8, "r":255, "b":0, "g":255, "text":"Paco", "size":2}},
{"screen":2, "slot":0, "object":{"type":"text", "visible":true, "x":9, "y":8, "r":0, "b":255, "g":255, "text":"Pris", "size":2}},
{"screen":3, "slot":0, "object":{"type":"text", "visible":true, "x":15, "y":8, "r":255, "b":255, "g":0, "text":"Viv", "size":2}},
{"screen":4, "slot":0, "object":{"type":"text", "visible":true, "x":15, "y":8, "r":255, "b":255, "g":0, "text":">:<", "size":2}}
]}'

# individual

#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":0, "slot":0, "object":{"type":"circle", "visible":true, "x":42, "y":18, "radius": 12, "filled": true, "r":0, "g":255, "b":0}}'
#sleep 0.3
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":0, "slot":1, "object":{"type":"rect", "visible":true, "x":46, "y":4, "w": 8, "h": 8, "filled": false, "r":0, "g":255, "b":255}}'
#sleep 0.3
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":0, "slot":2, "object":{"type":"text", "visible":true, "x":1, "y":8, "r":255, "g":255, "b":0, "text":"Welome to", "size":1}}'
#sleep 0.3
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":0, "slot":3, "object":{"type":"text", "visible":true, "x":5, "y":18, "r":255, "g":0, "b":255, "text":"the World!", "size":1}}'
#sleep 0.3
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":0, "slot":4, "object":{"type":"line", "visible":true, "x1":38, "y1":32, "x2":64, "y2":6, "r":128, "g":128, "b":128}}'
#sleep 0.3
#
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":1, "slot":0, "object":{"type":"text", "visible":true, "x":9, "y":8, "r":255, "g":0, "b":255, "text":"Paco", "size":2}}'
#sleep 0.3
#
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":2, "slot":0, "object":{"type":"text", "visible":true, "x":9, "y":8, "r":0, "g":255, "b":255, "text":"Pris", "size":2}}'
#sleep 0.3
#
#curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"screen":3, "slot":0, "object":{"type":"text", "visible":true, "x":15, "y":8, "r":255, "g":255, "b":0, "text":"Viv", "size":2}}'
#

# clear screen
# curl -X POST http://192.168.1.228/api/clearScreen?screen=0
