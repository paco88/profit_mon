#!/bin/sh

# curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"objects": [
# {"screen":1, "slot":0, "object":{"type":"text", "visible":true, "x":0, "y":10, "r":255, "b":0, "g":255, "text":"W", "size":2}},
# {"screen":1, "slot":1, "object":{"type":"text", "visible":true, "x":11, "y":10, "r":255, "b":0, "g":255, "text":"e", "size":2}},
# {"screen":1, "slot":2, "object":{"type":"text", "visible":true, "x":18, "y":10, "r":255, "b":0, "g":255, "text":"l", "size":2}},
# {"screen":1, "slot":3, "object":{"type":"text", "visible":true, "x":26, "y":10, "r":255, "b":0, "g":255, "text":"c", "size":2}},
# {"screen":1, "slot":4, "object":{"type":"text", "visible":true, "x":37, "y":10, "r":255, "b":0, "g":255, "text":"o", "size":2}},
# {"screen":1, "slot":5, "object":{"type":"text", "visible":true, "x":48, "y":10, "r":255, "b":0, "g":255, "text":"m", "size":2}},
# {"screen":1, "slot":6, "object":{"type":"text", "visible":true, "x":59, "y":10, "r":255, "b":0, "g":255, "text":"e", "size":2}}
# ]}'


curl -X POST http://192.168.1.228/api/object -H "Content-Type: application/json" -d '{"objects": [
	{"screen":1, "slot":0, "object":{"type":"text", "visible":true, "x":5, "y":3, "r":180, "b":80, "g":255, "text":"Wel", "size":2}},
	{"screen":1, "slot":1, "object":{"type":"text", "visible":true, "x":15, "y":16, "r":180, "b":255, "g":80, "text":"come", "size":2}}
]}'
