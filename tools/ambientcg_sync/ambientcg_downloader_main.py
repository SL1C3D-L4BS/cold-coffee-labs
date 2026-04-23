import csv
import os
import sys
import zipfile
from datetime import datetime, timezone

import requests
import yaml


def utc_ts() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


with open("config.yaml", "r") as f:
    config = yaml.safe_load(f)

resolution = config["resolution"]
file_type = config["file_type"]
unzip = config["unzip"]
unzipping_directory = config["unzip"]["folder"]
downloads_directory = config["downloads_folder"]

if resolution not in ["1K", "2K", "4K", "8K"] or file_type not in ["PNG", "JPG"]:
    print("Invalid format or type. Valid formats: 1k, 2k, 4k, 8k. Valid types: png, jpg.")
    sys.exit(1)

os.makedirs(downloads_directory, exist_ok=True)
os.makedirs(unzipping_directory, exist_ok=True)

csv_url = "https://ambientCG.com/api/v2/downloads_csv"
response = requests.get(csv_url)
csv_file = "downloads.csv"

with open(csv_file, "wb") as file:
    file.write(response.content)

# Exact tier match only: substring matching breaks e.g. resolution "2K" matching "12K-JPG".
target_attr = f"{resolution}-{file_type}"

work: list[tuple[dict, str]] = []
with open(csv_file, "r") as file:
    reader = csv.DictReader(file)
    for row in reader:
        attr = row["downloadAttribute"]
        if attr != target_attr and not (file_type == "JPG" and attr == resolution):
            continue
        download_url = row["downloadLink"]
        file_name = os.path.join(downloads_directory, download_url.split("file=")[1])
        work.append((row, file_name))

total = len(work)
print(
    f"[{utc_ts()}] ambientcg: {total} rows in scope (resolution={resolution} file_type={file_type})",
    flush=True,
)

for idx, (row, file_name) in enumerate(work, 1):
    aid = row.get("assetId", "?")
    attr = row["downloadAttribute"]
    base = os.path.basename(file_name)
    if os.path.exists(file_name):
        print(f"[{utc_ts()}] [{idx}/{total}] skip exists {aid} {attr} ({base})", flush=True)
        continue
    print(f"[{utc_ts()}] [{idx}/{total}] GET {aid} {attr} -> {base}", flush=True)
    zip_response = requests.get(row["downloadLink"])
    with open(file_name, "wb") as zip_file:
        zip_file.write(zip_response.content)
    sz = os.path.getsize(file_name)
    print(f"[{utc_ts()}] [{idx}/{total}] saved {base} ({sz} bytes)", flush=True)

if unzip:
    zips = sorted(f for f in os.listdir(downloads_directory) if f.lower().endswith(".zip"))
    uz_total = len(zips)
    print(f"[{utc_ts()}] unzip: {uz_total} archives", flush=True)
    for uz_idx, file in enumerate(zips, 1):
        dest = f"{unzipping_directory}/{file[:-4]}"
        with zipfile.ZipFile(f"{downloads_directory}/{file}", "r") as zip_ref:
            if not os.path.exists(dest):
                print(f"[{utc_ts()}] [unzip {uz_idx}/{uz_total}] extract {file}", flush=True)
                zip_ref.extractall(dest)
            else:
                print(f"[{utc_ts()}] [unzip {uz_idx}/{uz_total}] skip exists {file}", flush=True)


def delete_files(map_keyword: str):
    for directory in os.listdir(unzipping_directory):
        for file in os.listdir(os.path.join(unzipping_directory, directory)):
            if map_keyword in file:
                print(
                    f"Found {map_keyword} at {unzipping_directory}/{directory}. Deleting...",
                    flush=True,
                )
                os.remove(os.path.join(unzipping_directory, directory, file))


for k in config["keep_files"]:
    match k:
        case "color":
            if not config["keep_files"]["color"]:
                delete_files("Color")

        case "roughness":
            if not config["keep_files"]["roughness"]:
                delete_files("Roughness")

        case "normal_gl":
            if not config["keep_files"]["normal_gl"]:
                delete_files("NormalGL")

        case "normal_dx":
            if not config["keep_files"]["normal_dx"]:
                delete_files("NormalDX")

        case "metalness":
            if not config["keep_files"]["metalness"]:
                delete_files("Metalness")

        case "opacity":
            if not config["keep_files"]["opacity"]:
                delete_files("Opacity")

        case "ambient_occlusion":
            if not config["keep_files"]["ambient_occlusion"]:
                delete_files("AmbientOcclusion")

        case "displacement":
            if not config["keep_files"]["displacement"]:
                delete_files("Displacement")

        case "cover":
            if not config["keep_files"]["ambient_occlusion"]:
                for directory in os.listdir(unzipping_directory):
                    for file in os.listdir(os.path.join(unzipping_directory, directory)):
                        if not any(
                            keyword in file
                            for keyword in [
                                "Color",
                                "Roughness",
                                "NormalGL",
                                "NormalDX",
                                "Metalness",
                                "AmbientOcclusion",
                                "Opacity",
                                "Displacement",
                            ]
                        ):
                            print(
                                f"Found {k} at {unzipping_directory}/{directory}. Deleting...",
                                flush=True,
                            )
                            os.remove(os.path.join(unzipping_directory, directory, file))

print(f"[{utc_ts()}] ambientcg main.py finished.", flush=True)
