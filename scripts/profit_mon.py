"""
This script gets netliqvalue data from database and publish to profit monitor LED panel.
"""
from collections import defaultdict
from datetime import date, datetime, timedelta
from enum import IntEnum
import json
import math
import pymysql
import pandas as pd
import requests
import time
import warnings

warnings.filterwarnings('ignore')

# Constants
eod_time = "23:00:00"
panel_max_res_x = 64
panel_max_res_y = 32
base_unit_pl = 10000
MAX_SCALE = 5  # Number of scales is MAX_SCALE + 1
dot_radius = 2
dot_colour_1 = (90, 90, 90)
dot_colour_2 = (64, 64, 128)
screen = 7
ESP32_IP = "192.168.1.228"
BASE_URL = f"http://{ESP32_IP}"

class TimeFrame(IntEnum):
	DAY = 0
	WEEK = 1
	MONTH = 2
	YEAR = 3

screens = {
	TimeFrame.DAY: 4,
	TimeFrame.WEEK: 5,
	TimeFrame.MONTH: 6,
	TimeFrame.YEAR: 7,
}

# TimeFrame -> count
update_counts = defaultdict(int)


def get_data(time_frame):
	db = pymysql.connect(read_default_file='/home/paco/.mypass')

	df = None

	if time_frame == TimeFrame.DAY:
		prev_date = date.today() - timedelta(days=1)
		prev_eod = prev_date.isoformat() + " " + eod_time
		df = pd.read_sql(f"select high as h, low as l, close as c from price where seccode='netLiqValue_U10600296' and `interval`='T' and utcdate>='{prev_eod}' order by utcDate", db)

	elif time_frame == TimeFrame.WEEK:
		today = date.today()
		monday = today - timedelta(days=today.weekday())
		monday_sod = monday.isoformat() + " 00:00:00"
		df = pd.read_sql(f"select utcDate as datetime, high as h, low as l, close as c from price where seccode='netLiqValue_U10600296' and `interval`='T' and utcdate>='{monday_sod}' order by utcDate", db)
		df = df.set_index("datetime").resample("2H").agg({"h": "max", "l": "min", "c": "last"}).dropna().reset_index()[["h", "l", "c"]]

	elif time_frame == TimeFrame.MONTH:
		month_start_date = date.today().replace(day=1)
		month_start_sod = month_start_date.isoformat() + " 00:00:00"
		df = pd.read_sql(f"select utcDate as datetime, high as h, low as l, close as c from price where seccode='netLiqValue_U10600296' and `interval`='T' and utcdate>='{month_start_sod}' order by utcDate", db)
		df = df.set_index("datetime").resample("D").agg({"h": "max", "l": "min", "c": "last"}).dropna().reset_index()[["h", "l", "c"]]

	elif time_frame == TimeFrame.YEAR:
		year_start_date = date.today().replace(month=1, day=1)
		year_start_sod = year_start_date.isoformat() + " 00:00:00"
		df = pd.read_sql(f"select utcDate as datetime, high as h, low as l, close as c from price where seccode='netLiqValue_U10600296' and `interval`='T' and utcdate>='{year_start_sod}' order by utcDate", db)
		df = df.set_index("datetime").resample("W").agg({"h": "max", "l": "min", "c": "last"}).dropna().reset_index()[["h", "l", "c"]]

	else:
		raise ValueError("Invalid time frame: " + str(time_frame))

	db.close()
	return df


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


def update_chart(time_frame):
	global update_counts

	screen = screens[time_frame]

	# Get data
	df = get_data(time_frame)

	if len(df) < 2:
		clear_screen_data = {"screen": screen}
		post(f"/api/clearScreen", clear_screen_data)

		payload = [
			{"screen": screen, "slot": 0, "object": {"type": "text", "x": 1, "y": 5, "r": 0, "g": 255, "b": 0, "text": "Profit"}},
			{"screen": screen, "slot": 1, "object": {"type": "text", "x": 3, "y": 14, "r": 0, "g": 0, "b": 255, "text": "Monitor"}},
		]
		post(f"/api/object", {"objects": payload})
		return

	# Get pl relative to previous end of day
	prev_close = df.iloc[0].c
	df = df - prev_close

	# convert to HKD
	hkd_rate = 7.83
	df *= hkd_rate

	# drop the overnight row and get at max the most recent rows that fits the LED panel.
	df = df.iloc[1:][-panel_max_res_x:]

	def round_(x) -> int:
			return math.floor(abs(x)) * (1 if x > 0 else -1)

	# convert pl's into units
	c_orig = df.c
	df = df.applymap(lambda x: round_(x / base_unit_pl))

	# calculate scale, scale is exponential from 0 to MAX_SCALE, meaning pl from 2^0 to 2^MAX_SCALE units per pixel
	max_units = max(df.h.max(), 0)
	min_units = min(df.l.min(), 0)
	num_units = max_units - min_units + 1
	scale = max(min(math.ceil((math.log(num_units) - math.log(panel_max_res_y)) / math.log(2)), MAX_SCALE), 0)
	units_per_pixel = int(math.pow(2, scale))

	# convert pl's into pixels
	df = df.applymap(lambda x: round_(x / units_per_pixel))
	df['c_orig'] = c_orig

	# calculate base
	max_pixels = round_(max_units / units_per_pixel)
	min_pixels = round_(min_units / units_per_pixel)
	base = math.floor((panel_max_res_y + 1) / 2 - (max_pixels + min_pixels) / 2)

	# create chart dataframe
	chart_df = df[['h','l']].applymap(lambda x: int(x + base))
	chart_df['c'] = df.apply(lambda r: -255 if r.c == 0 and r.c_orig < 0 else (r.c + base), axis=1)

	# Send chart to panel
	chart_dict = chart_df.to_dict(orient="records")
	chart_obj = {"type": "chart", "base": base, "bars": chart_dict}
	chart_data = {"screen": screen, "slot": 9, "object": chart_obj}
	print(f"{datetime.now()} {json.dumps(chart_data)}")
	post("/api/object", chart_data)

	# Send scale dots to panel, up to 6 dots

	batch = []
	update_counts[time_frame] += 1

	for slot in range(MAX_SCALE + 1):
		colour = (0, 0, 0) if slot > scale else dot_colour_1 if update_counts[time_frame] % 2 == 1 else dot_colour_2
		x = panel_max_res_x - dot_radius - 2 - (dot_radius * 2 + 2) * slot
		circle_obj = {"type": "circle", "x": x, "y": dot_radius, "radius": dot_radius, "filled": True, "r": colour[0], "g": colour[1], "b": colour[2]}
		batch.append({"screen": screen, "slot": slot, "object": circle_obj})

	# Draw the chart number
	line_obj = {"type": "line", "x1": 0, "y1": 0, "x2": time_frame.value, "y2": 0, "r": 255, "g": 255, "b": 255}
	batch.append({"screen": screen, "slot": slot + 1, "object": line_obj})

	batch_data = {"objects": batch}
	print(f"{datetime.now()} {json.dumps(batch_data)}")
	post("/api/object", batch_data)


if __name__ == "__main__":
	tick = 0

	while True:
		tick += 1

		update_chart(TimeFrame.DAY)
		time.sleep(5)

		if tick % 15 == 1:
			update_chart(TimeFrame.WEEK)

		elif tick % 15 == 2:
			update_chart(TimeFrame.MONTH)

		elif tick % 15 == 3:
			update_chart(TimeFrame.YEAR)

		time.sleep(21)
