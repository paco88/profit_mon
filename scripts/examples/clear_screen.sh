#!/bin/sh

if [ $# -ne 1 ]; then
  echo "Usage: $0 <screen_number>"
  exit 1
fi

screen="$1"

curl -X POST "http://192.168.1.228/api/clearScreen?screen=${screen}"
