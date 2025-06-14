import subprocess
import time
import sys
import requests
import matplotlib.pyplot as plt
import pandas as pd
from datetime import datetime, date
import os
from tabulate import tabulate

wifi_ssid = "ESP32_Hotspot"
esp_ip = "http://192.168.4.1"
data_url = f"{esp_ip}/data"
ack_url = f"{esp_ip}/ack"

pressure_values = []
depth_values = []
timestamps = []

SAVE_DIR = os.path.expanduser("/home/waheeb/Desktop/Scripts/Float/Readings")
os.makedirs(SAVE_DIR, exist_ok=True)

def calibrated_depth(pressure_kpa):
    if pressure_kpa <= 102:
        return (pressure_kpa - 100) * (0.2 - 0) / (102 - 100)
    elif pressure_kpa <= 105:
        return 0.2 + (pressure_kpa - 102) * (0.5 - 0.2) / (105 - 102)
    elif pressure_kpa <= 120:
        return 0.5 + (pressure_kpa - 105) * (2.0 - 0.5) / (120 - 105)
    elif pressure_kpa <= 125:
        return 2.0 + (pressure_kpa - 120) * (2.5 - 2.0) / (125 - 120)
    elif pressure_kpa <= 130:
        return 2.0 + (pressure_kpa - 125) * (3.0 - 2.5) / (130 - 125)
    elif pressure_kpa <= 134:
        return 3.0 + (pressure_kpa - 130) * (3.9 - 3.0) / (134 - 130)
    else:
        return 3.9 + (pressure_kpa - 134) * (3.9 - 3.0) / (134 - 130)

def get_available_networks():
    try:
        result = subprocess.run(
            ["nmcli", "-t", "-f", "SSID", "dev", "wifi"],
            capture_output=True, text=True, check=True
        )
        return result.stdout.lower()
    except subprocess.CalledProcessError:
        return ""

def is_esp32_hotspot_available():
    return wifi_ssid.lower() in get_available_networks()

def connect_to_wifi():
    print(f"Trying to connect to {wifi_ssid}...")
    try:
        subprocess.run(["nmcli", "dev", "wifi", "connect", wifi_ssid], check=True)
        print(f"Connected to {wifi_ssid}. Waiting for network to stabilize...")
        time.sleep(5)
    except subprocess.CalledProcessError:
        print(f"Failed to connect to {wifi_ssid}. Retrying...")

def fetch_data():
    try:
        response = requests.get(data_url, timeout=3)
        if response.status_code == 200:
            data = response.text.strip()
            print("Received Data:", data)

            if len(data) == 15:
                pressure, time_str = data.split('/')
                pressure = float(pressure)
                time_obj = datetime.strptime(time_str, "%H:%M:%S").time()
                full_datetime = datetime.combine(date.today(), time_obj)
                depth = round(calibrated_depth(pressure), 2)

                initial_data = [
                    ["0", "EXP05", full_datetime.strftime("%H:%M:%S"), f"{pressure:.2f}", "0.00m"]
                ]
                headers = ["Number", "Company name", "Time", "Pressure (kPa)", "Depth (m)"]

                print(tabulate(initial_data, headers=headers, tablefmt="grid"))

            else:
                for entry in data.split(", "):
                    try:
                        pressure, time_str = entry.split("/")
                        pressure = float(pressure)
                        time_obj = datetime.strptime(time_str, "%H:%M:%S").time()
                        full_datetime = datetime.combine(date.today(), time_obj)

                        depth = round(calibrated_depth(pressure), 2)

                        if full_datetime not in timestamps:
                            timestamps.append(full_datetime)
                            pressure_values.append(pressure)
                            depth_values.append(depth)
                    except ValueError:
                        print(f"Skipping invalid data entry: {entry}")

            ack_response = requests.get(ack_url, timeout=3)
            if ack_response.status_code == 200:
                print("Sent acknowledgment to ESP32.")
        else:
            print(f"Failed to get data. Status Code: {response.status_code}")
    except requests.RequestException as e:
        print(f"Error fetching data: {e}")

def plot_and_save():
    if not timestamps:
        print("No data to plot.")
        return

    sorted_data = sorted(zip(timestamps, pressure_values, depth_values), key=lambda x: x[0])
    sorted_timestamps, sorted_pressures, sorted_depths = zip(*sorted_data)

    fig, axs = plt.subplots(2, 1, figsize=(10, 8))

    axs[0].plot(sorted_timestamps, sorted_depths, linestyle="-", color="b", label="Depth (m)")
    axs[0].scatter(sorted_timestamps, sorted_depths, color="r")
    axs[0].set_xlabel("Time", labelpad=0)
    axs[0].set_ylabel("Depth (m)")
    axs[0].set_title("ESP32 Depth Data Over Time")
    axs[0].legend()
    axs[0].tick_params(axis='x', rotation=45)
    axs[0].grid()

    company_names = ["EXP05"] * len(sorted_timestamps)
    numbers = list(range(1, len(sorted_timestamps) + 1))
    df = pd.DataFrame({
        "Number": numbers,
        "Company name": company_names,
        "Time": [t.strftime("%H:%M:%S") for t in sorted_timestamps],
        "Pressure (kPa)": sorted_pressures,
        "Depth (m)": sorted_depths
    })

    last_df = df.tail(30).reset_index(drop=True)

    table1 = last_df.iloc[:15]
    table2 = last_df.iloc[15:]

    axs[1].axis("off")

    table_left = axs[1].table(cellText=table1.values, colLabels=table1.columns,
                              cellLoc="center", loc="upper left", bbox=[0.1, -0.2, 0.4, 1.2])

    table_right = axs[1].table(cellText=table2.values, colLabels=table2.columns,
                               cellLoc="center", loc="upper left", bbox=[0.55, -0.2, 0.4, 1.2])

    for table in [table_left, table_right]:
        table.auto_set_font_size(False)
        table.set_fontsize(10)
        table.auto_set_column_width([0, 1, 2, 3, 4])

    print("\n" + tabulate(last_df.values, headers=last_df.columns, tablefmt="grid"))

    mng = plt.get_current_fig_manager()
    try:
        mng.full_screen_toggle()
    except AttributeError:
        try:
            mng.window.state('zoomed')
        except:
            pass
    plt.pause(0.5)

    now = datetime.now()
    filename_base = f"{now.hour}:{now.minute}-{now.day}-{now.month}"

    png_path = os.path.join(SAVE_DIR, f"{filename_base}.png")
    fig.savefig(png_path, bbox_inches='tight')
    print(f"Saved graph to: {png_path}")

    txt_path = os.path.join(SAVE_DIR, f"{filename_base}.txt")
    last_df.to_csv(txt_path, sep="\t", index=False)
    print(f"Saved data to: {txt_path}")

    plt.show()
    time.sleep(3)
    subprocess.run(["nmcli", "radio", "wifi", "off"])
    print("WiFi turned off.")

if __name__ == "__main__":
    while True:
        if is_esp32_hotspot_available():
            connect_to_wifi()
            fetch_data()
            plot_and_save()
            sys.exit(0)
        else:
            print(f"{wifi_ssid} not found. Scanning again...")
        time.sleep(3)
