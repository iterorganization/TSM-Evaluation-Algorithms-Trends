#!/usr/bin/env python3
"""Plot sensor measurements and estimates from projection history files.

This script creates one figure per projection CSV file and overlays the
following series for each requested sensor:
  - measured_value
  - raw_estimate
  - estimate
  - projected_4K_0kA
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List

import matplotlib.pyplot as plt


REQUIRED_COLUMNS = (
    "step_index",
    "sensor_id",
    "measured_value",
    "raw_estimate",
    "estimate",
    "projected_4K_0kA",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Create one plot per projection CSV file for one or more sensors, "
            "comparing measurement, raw estimate, estimate, and projected_4K_0kA."
        )
    )
    parser.add_argument(
        "--sensors",
        nargs="+",
        help="Sensor IDs to plot (if omitted, all sensors found in projection files are used)",
    )
    parser.add_argument(
        "--data-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "data",
        help="Directory containing projection CSV files",
    )
    parser.add_argument(
        "--pattern",
        default="projection_history*.csv",
        help="Glob pattern used to find projection CSV files",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "plots",
        help="Directory where PNG plots will be written",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=140,
        help="Output image DPI",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Display plots interactively in addition to saving them",
    )
    return parser.parse_args()


def load_sensor_rows(csv_path: Path, sensors: List[str]) -> Dict[str, Dict[str, List[float]]]:
    sensor_set = set(sensors)
    series = {
        sensor: {
            "step_index": [],
            "measured_value": [],
            "raw_estimate": [],
            "estimate": [],
            "projected_4K_0kA": [],
        }
        for sensor in sensors
    }

    with csv_path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        missing = [column for column in REQUIRED_COLUMNS if column not in (reader.fieldnames or [])]
        if missing:
            raise ValueError(f"{csv_path.name} is missing required columns: {', '.join(missing)}")

        for row in reader:
            sensor_id = row["sensor_id"]
            if sensor_id not in sensor_set:
                continue

            sensor_data = series[sensor_id]
            sensor_data["step_index"].append(float(row["step_index"]))
            sensor_data["measured_value"].append(float(row["measured_value"]))
            sensor_data["raw_estimate"].append(float(row["raw_estimate"]))
            sensor_data["estimate"].append(float(row["estimate"]))
            sensor_data["projected_4K_0kA"].append(float(row["projected_4K_0kA"]))

    for sensor in sensors:
        if not series[sensor]["step_index"]:
            continue
        zipped = sorted(
            zip(
                series[sensor]["step_index"],
                series[sensor]["measured_value"],
                series[sensor]["raw_estimate"],
                series[sensor]["estimate"],
                series[sensor]["projected_4K_0kA"],
            ),
            key=lambda values: values[0],
        )
        (
            series[sensor]["step_index"],
            series[sensor]["measured_value"],
            series[sensor]["raw_estimate"],
            series[sensor]["estimate"],
            series[sensor]["projected_4K_0kA"],
        ) = map(list, zip(*zipped))

    return series


def collect_sensors_from_files(projection_files: List[Path]) -> List[str]:
    ordered_sensors: List[str] = []
    seen = set()
    for csv_path in projection_files:
        with csv_path.open("r", newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            if "sensor_id" not in (reader.fieldnames or []):
                raise ValueError(f"{csv_path.name} is missing required column: sensor_id")
            for row in reader:
                sensor_id = row["sensor_id"]
                if sensor_id in seen:
                    continue
                seen.add(sensor_id)
                ordered_sensors.append(sensor_id)
    return ordered_sensors


def build_sensor_plot(
    sensor: str,
    projection_files: List[Path],
    sensor_data_by_file: Dict[Path, Dict[str, Dict[str, List[float]]]],
    output_path: Path,
) -> None:
    fig, axes = plt.subplots(len(projection_files), 1, figsize=(12, 3.6 * len(projection_files)), sharex=False)
    if len(projection_files) == 1:
        axes = [axes]

    for axis, csv_path in zip(axes, projection_files):
        sensor_data = sensor_data_by_file[csv_path][sensor]
        if not sensor_data["step_index"]:
            axis.text(0.5, 0.5, f"No data for sensor {sensor}", ha="center", va="center")
            axis.set_axis_off()
            continue

        axis.plot(sensor_data["step_index"], sensor_data["measured_value"], label="measured_value", linewidth=1.8)
        axis.plot(sensor_data["step_index"], sensor_data["raw_estimate"], label="raw_estimate", linewidth=1.5)
        axis.plot(sensor_data["step_index"], sensor_data["estimate"], label="estimate", linewidth=1.5)
        axis.plot(sensor_data["step_index"], sensor_data["projected_4K_0kA"], label="projected_4K_0kA", linewidth=1.8)

        axis.set_title(csv_path.name)
        axis.set_ylabel("Value")
        axis.set_xlabel("step_index")
        axis.grid(True, alpha=0.25)
        axis.legend(loc="best")

    fig.suptitle(f"Projection comparison for sensor: {sensor}")
    fig.tight_layout()
    fig.savefig(output_path)
    plt.close(fig)


def main() -> int:
    args = parse_args()

    projection_files = sorted(args.data_dir.glob(args.pattern))
    if not projection_files:
        raise FileNotFoundError(
            f"No projection files found in {args.data_dir} matching pattern '{args.pattern}'"
        )

    args.output_dir.mkdir(parents=True, exist_ok=True)

    sensors = args.sensors if args.sensors else collect_sensors_from_files(projection_files)
    if not sensors:
        raise ValueError("No sensors found in projection files")

    sensor_data_by_file = {
        csv_path: load_sensor_rows(csv_path, sensors)
        for csv_path in projection_files
    }

    for sensor in sensors:
        output_path = args.output_dir / f"{sensor}__projection_comparison.png"
        build_sensor_plot(sensor, projection_files, sensor_data_by_file, output_path)
        print(f"Saved: {output_path}")

    if args.show:
        plt.show()
    else:
        plt.close("all")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())