"""Re-plot the logged experiment data and recompute the headline numbers.

Usage:  python analysis/plot_results.py
Needs:  numpy, pandas, matplotlib
Writes: docs/images/pid_step_response.png, docs/images/phase_transitions.png
"""
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
OUT = ROOT / "docs" / "images"
OUT.mkdir(parents=True, exist_ok=True)

# Palette: first three categorical slots + neutral text/grid tones
BLUE, ORANGE, AQUA = "#2a78d6", "#eb6834", "#1baf7a"
INK, INK2, GRID, SURFACE = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"

plt.rcParams.update({
    "font.family": "DejaVu Sans", "font.size": 10,
    "axes.edgecolor": GRID, "axes.labelcolor": INK2, "axes.titlecolor": INK,
    "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "xtick.color": INK2, "ytick.color": INK2, "axes.grid": True,
    "grid.color": GRID, "grid.linewidth": 0.8,
    "axes.spines.top": False, "axes.spines.right": False,
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE,
    "legend.frameon": False, "lines.linewidth": 2,
})

SETPOINT = 20.0


def load_pid(name):
    return pd.read_csv(DATA / "pid_tuning" / f"{name}.csv")


def settle_time(df, band=1.0):
    """First time after which the trace stays inside setpoint ± band."""
    outside = (df.temp_c - SETPOINT).abs() > band
    if not outside.any():
        return 0.0
    last_out = outside[outside].index.max()
    return float(df.time_s.iloc[min(last_out + 1, len(df) - 1)])


# ---------------------------------------------------------------- PID response
runs = [
    ("kp55_kd0_ki0", "P only (Kp 55)", ORANGE),
    ("kp55_kd0_ki0.6", "PI (Kp 55, Ki 0.6)", AQUA),
    ("kp55_kd2.5_ki0.6", "PID, final (Kp 55, Ki 0.6, Kd 2.5)", BLUE),
]
fig, ax = plt.subplots(figsize=(8, 4.2), dpi=150)
ax.axhspan(SETPOINT - 1, SETPOINT + 1, color=GRID, alpha=0.6, lw=0, label="±1 °C band")
ax.axhline(SETPOINT, color=INK2, lw=1, ls="--")
ax.text(1, SETPOINT + 0.15, "setpoint 20 °C", color=INK2, fontsize=9, va="bottom")
for name, label, color in runs:
    d = load_pid(name)
    ax.plot(d.time_s, d.temp_c, color=color, label=label)

final = load_pid("kp55_kd2.5_ki0.6")
ts = settle_time(final)
steady = final[final.time_s >= final.time_s.iloc[-1] - 120].temp_c
print(f"Final PID: settles within +/-1 C after {ts:.1f} s; "
      f"last 120 s mean {steady.mean():.2f} C, std {steady.std():.2f} C, "
      f"max |dev| {(steady - SETPOINT).abs().max():.2f} C")
ax.set_title("Cooling step response, 30 °C → 20 °C")
ax.set_xlabel("Time (s)")
ax.set_ylabel("Sample temperature (°C)")
ax.legend(loc="upper right", fontsize=9)
fig.tight_layout()
fig.savefig(OUT / "pid_step_response.png")
plt.close(fig)


# ---------------------------------------------------------- Phase transitions
def heating_rate(df, window=180):
    """Smoothed heating rate in °C/min against smoothed temperature."""
    t = df.temp_c.rolling(window, center=True).mean()
    rate = pd.Series(np.gradient(t, df.time_s) * 60).rolling(window, center=True).mean()
    return t, rate


def dips(t, rate, t_min, t_max, keep, prominence=0.1, span=400):
    """The `keep` deepest local minima of the heating rate inside [t_min, t_max]."""
    found = []  # (rate, temperature)
    for i in range(span, len(rate) - span):
        if not (t_min < t[i] < t_max) or np.isnan(rate[i]):
            continue
        if rate[i] == rate[i - span:i + span].min():
            prom = min(rate[i - span:i].max(), rate[i:i + span].max()) - rate[i]
            if prom > prominence and all(abs(t[i] - f) > 3 for _, f in found):
                found.append((rate[i], float(t[i])))
    return sorted(f for _, f in sorted(found)[:keep])


samples = [  # file, title, search range (°C), number of transitions expected
    ("pf127_17wt_heating", "Pluronic F-127, 17 wt% (sol → gel → sol)", (15, 60), 2),
    ("paraffin_wax_heating", "Paraffin wax (melting)", (30, 70), 1),
]
fig, axes = plt.subplots(1, 2, figsize=(10, 4), dpi=150)
for ax, (name, title, (lo, hi), keep) in zip(axes, samples):
    d = pd.read_csv(DATA / "phase_transition" / f"{name}.csv")
    t, rate = heating_rate(d)
    ax.plot(t, rate, color=BLUE)
    for x in dips(t.values, rate.values, lo, hi, keep):
        y = rate[(t - x).abs().idxmin()]
        ax.annotate(f"{x:.1f} °C", (x, y), xytext=(0, -26), textcoords="offset points",
                    ha="center", color=INK, fontsize=9,
                    arrowprops=dict(arrowstyle="-", color=INK2, lw=1))
        ax.plot([x], [y], "o", ms=8, color=BLUE, mec=SURFACE, mew=2)
        print(f"{name}: heating-rate dip at {x:.1f} C")
    ax.set_title(title)
    ax.set_xlabel("Sample temperature (°C)")
    ax.set_xlim(t.min(), t.max())
axes[0].set_ylabel("Heating rate (°C/min)")
fig.suptitle("Latent heat shows up as a dip in the heating rate", x=0.01, ha="left",
             fontsize=12, fontweight="bold", color=INK)
fig.tight_layout()
fig.savefig(OUT / "phase_transitions.png")
plt.close(fig)
