#!/usr/bin/env python3
"""DTA400 project: one simple figure.

Usage: python3 simple_figure.py results.csv

For each scenario: the mean transfer time of the best setting
(InitialCwnd 10, SegmentSize 1448, DelAckCount 1), and of the same setting
with ONE parameter changed. Mean over all runs (30 runs, or 1 for home).
"""
import csv
import statistics
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

rows = list(csv.DictReader(open(sys.argv[1])))


def mean_time(scenario, cwnd, seg, delack):
    times = [float(r["transferTime"]) for r in rows
             if r["scenario"] == scenario and int(r["initCwnd"]) == cwnd
             and int(r["segmentSize"]) == seg and int(r["delAckCount"]) == delack]
    return statistics.mean(times)


# (label, InitialCwnd, SegmentSize, DelAckCount)
SETTINGS = [
    ("Best", 10, 1448, 1),
    ("", 1, 1448, 1),
    ("", 10, 536, 1),
    ("", 10, 1448, 4),
]
SCENARIOS = [("home", "Home"), ("mobile", "Mobile"), ("australia", "Long distance")]
COLORS = ["#2a78d6", "#8d8c88", "#8d8c88", "#8d8c88"]  # best in blue, others gray

plt.rcParams.update({"font.family": "serif", "font.size": 8})
fig, axes = plt.subplots(1, 3, figsize=(7.16, 2.7))
for ax, (sc, title) in zip(axes, SCENARIOS):
    values = [mean_time(sc, c, s, d) for _, c, s, d in SETTINGS]
    bars = ax.bar(range(4), values, color=COLORS, width=0.65)
    ax.bar_label(bars, fmt="%.2f s", fontsize=7)  # write the value on each bar
    ax.set_xticks(range(4))
    # Under every bar: its three parameter values (small text)
    labels = [f"{name}\nCwnd={c}\nSeg={g}\nDelAck={d}".lstrip("\n")
              for name, c, g, d in SETTINGS]
    ax.set_xticklabels(labels, fontsize=6)
    ax.set_title(title)
    ax.set_ylim(0, max(values) * 1.18)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
axes[0].set_ylabel("Mean transfer time (s)")
fig.tight_layout()
fig.savefig("fig1-simple.pdf")
fig.savefig("fig1-simple.png", dpi=300)
print("saved fig1-simple.pdf and fig1-simple.png")
