#!/usr/bin/env python3

import argparse
import csv
import math
from pathlib import Path


def load_rows(path: Path):
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        return list(reader)


def group_points(rows, metric):
    grouped = {}
    for row in rows:
        algorithm = row["algorithm"]
        point = (int(row["vertices"]), float(row[metric]))
        grouped.setdefault(algorithm, []).append(point)

    for points in grouped.values():
        points.sort()
    return grouped


def svg_line_chart(grouped, title, y_label, output_path: Path):
    width = 900
    height = 560
    margin_left = 80
    margin_right = 40
    margin_top = 70
    margin_bottom = 70
    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom
    colors = {
        "Dijkstra": "#0f766e",
        "Bellman-Ford": "#b45309",
    }

    all_points = [point for points in grouped.values() for point in points]
    if not all_points:
        raise SystemExit("No successful benchmark rows were found in the CSV.")

    min_x = min(point[0] for point in all_points)
    max_x = max(point[0] for point in all_points)
    max_y = max(point[1] for point in all_points)

    if min_x == max_x:
        min_x -= 1
        max_x += 1
    if math.isclose(max_y, 0.0):
        max_y = 1.0

    def x_pos(value):
        return margin_left + ((value - min_x) / (max_x - min_x)) * plot_width

    def y_pos(value):
        return margin_top + plot_height - (value / max_y) * plot_height

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        '<rect width="100%" height="100%" fill="#fffaf5"/>',
        f'<text x="{width / 2}" y="32" text-anchor="middle" font-size="24" fill="#1f2937">{title}</text>',
        f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#374151" stroke-width="2"/>',
        f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{margin_left + plot_width}" y2="{margin_top + plot_height}" stroke="#374151" stroke-width="2"/>',
        f'<text x="{width / 2}" y="{height - 20}" text-anchor="middle" font-size="16" fill="#374151">Number of vertices</text>',
        f'<text x="24" y="{height / 2}" text-anchor="middle" font-size="16" fill="#374151" transform="rotate(-90 24 {height / 2})">{y_label}</text>',
    ]

    for tick in range(6):
        y_value = max_y * tick / 5
        y = y_pos(y_value)
        svg.append(f'<line x1="{margin_left}" y1="{y}" x2="{margin_left + plot_width}" y2="{y}" stroke="#e5e7eb" stroke-width="1"/>')
        svg.append(f'<text x="{margin_left - 12}" y="{y + 5}" text-anchor="end" font-size="12" fill="#4b5563">{y_value:.2f}</text>')

    x_ticks = sorted({point[0] for point in all_points})
    for value in x_ticks:
        x = x_pos(value)
        svg.append(f'<line x1="{x}" y1="{margin_top + plot_height}" x2="{x}" y2="{margin_top + plot_height + 6}" stroke="#374151" stroke-width="1"/>')
        svg.append(f'<text x="{x}" y="{margin_top + plot_height + 24}" text-anchor="middle" font-size="12" fill="#4b5563">{value}</text>')

    legend_y = margin_top - 28
    legend_x = margin_left
    for index, (algorithm, points) in enumerate(grouped.items()):
        color = colors.get(algorithm, "#2563eb")
        svg.append(f'<rect x="{legend_x + index * 160}" y="{legend_y}" width="18" height="18" fill="{color}" rx="3"/>')
        svg.append(
            f'<text x="{legend_x + 26 + index * 160}" y="{legend_y + 14}" font-size="14" fill="#374151">{algorithm}</text>'
        )

        polyline = " ".join(f"{x_pos(x)},{y_pos(y)}" for x, y in points)
        svg.append(f'<polyline points="{polyline}" fill="none" stroke="{color}" stroke-width="3"/>')

        for x_value, y_value in points:
            svg.append(f'<circle cx="{x_pos(x_value)}" cy="{y_pos(y_value)}" r="4.5" fill="{color}"/>')

    svg.append("</svg>")
    output_path.write_text("\n".join(svg), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Create SVG charts from SSSP benchmark CSV output.")
    parser.add_argument("--input", required=True, help="CSV file produced by the benchmark program")
    parser.add_argument("--outdir", default="plots", help="Output directory for SVG charts")
    args = parser.parse_args()

    input_path = Path(args.input)
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    rows = load_rows(input_path)
    runtime_points = group_points(rows, "runtime_ms")
    memory_points = group_points(rows, "total_memory_bytes")

    svg_line_chart(runtime_points, "SSSP Runtime Comparison", "Runtime (ms)", outdir / "runtime_ms.svg")
    svg_line_chart(memory_points, "SSSP Memory Comparison", "Total memory (bytes)", outdir / "total_memory_bytes.svg")


if __name__ == "__main__":
    main()
