from pathlib import Path
import csv
import warnings

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import font_manager
import numpy as np

warnings.filterwarnings("error", message=r"Glyph .* missing from current font\.")

ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "experiments" / "ik_algorithm_comparison_1000.csv"
OUTPUT_STEM = Path(__file__).resolve().parent / "fig5_ik_algorithm_comparison"
CJK_FONT = Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
if CJK_FONT.exists():
    font_manager.fontManager.addfont(str(CJK_FONT))
    CJK_FAMILY = font_manager.FontProperties(fname=str(CJK_FONT)).get_name()
else:
    CJK_FAMILY = "Noto Sans CJK SC"

mpl.rcParams.update(
    {
        "font.family": "sans-serif",
        "font.sans-serif": [CJK_FAMILY, "Microsoft YaHei", "SimHei", "DejaVu Sans", "sans-serif"],
        "svg.fonttype": "none",
        "pdf.fonttype": 42,
        "font.size": 7,
        "axes.titlesize": 8,
        "axes.labelsize": 7,
        "xtick.labelsize": 6.5,
        "ytick.labelsize": 6.5,
        "axes.spines.right": False,
        "axes.spines.top": False,
        "axes.linewidth": 0.7,
        "figure.dpi": 150,
    }
)


def read_rows():
    with DATA_PATH.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def label_bars(ax, bars, formatter):
    for bar in bars:
        value = bar.get_height()
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            value,
            formatter(value),
            ha="center",
            va="bottom",
            fontsize=6.2,
            color="#202124",
        )


def style_axis(ax, panel_label, title, ylabel):
    ax.text(
        -0.16,
        1.08,
        panel_label,
        transform=ax.transAxes,
        fontsize=8,
        fontweight="bold",
        va="top",
    )
    ax.set_title(title, loc="left", fontweight="bold", pad=7)
    ax.set_ylabel(ylabel)
    ax.grid(axis="y", color="#D9DDE3", linewidth=0.55, alpha=0.8)
    ax.set_axisbelow(True)


def main():
    rows = read_rows()
    methods = ["JT", "PINV", "DLS"]
    colors = ["#4C78A8", "#E6A04B", "#4C9F70"]
    hatches = ["///", "\\\\\\", "..."]
    x = np.arange(len(methods))

    success = np.array([float(row["success_rate"]) * 100 for row in rows])
    runtime = np.array([float(row["runtime_mean_ms"]) for row in rows])
    iterations = np.array([float(row["iterations_mean"]) for row in rows])
    position_mm = np.array([float(row["position_error_mean_m"]) * 1000 for row in rows])

    fig, axes = plt.subplots(2, 2, figsize=(7.1, 4.7), constrained_layout=True)
    panels = [
        (axes[0, 0], success, "a", "收敛可靠性", "成功率（%）"),
        (axes[0, 1], runtime, "b", "计算代价", "平均耗时（ms）"),
        (axes[1, 0], iterations, "c", "迭代需求", "平均迭代次数"),
        (axes[1, 1], position_mm, "d", "收敛样本精度", "平均位置误差（mm）"),
    ]

    for ax, values, label, title, ylabel in panels:
        bars = ax.bar(
            x,
            values,
            width=0.64,
            color=colors,
            edgecolor="#30343B",
            linewidth=0.55,
        )
        for bar, hatch in zip(bars, hatches):
            bar.set_hatch(hatch)
        ax.set_xticks(x, methods)
        style_axis(ax, label, title, ylabel)

    axes[0, 0].set_ylim(0, 105)
    label_bars(axes[0, 0], axes[0, 0].patches, lambda value: f"{value:.1f}%")
    axes[0, 1].set_ylim(0, runtime.max() * 1.18)
    label_bars(axes[0, 1], axes[0, 1].patches, lambda value: f"{value:.2f}")
    axes[1, 0].set_ylim(0, iterations.max() * 1.14)
    label_bars(axes[1, 0], axes[1, 0].patches, lambda value: f"{value:.2f}")
    axes[1, 1].set_ylim(0, position_mm.max() * 1.18)
    label_bars(axes[1, 1], axes[1, 1].patches, lambda value: f"{value:.2f}")

    fig.suptitle(
        "相同 1000 组样本条件下的逆运动学算法比较",
        fontsize=9,
        fontweight="bold",
    )
    fig.text(
        0.5,
        -0.01,
        "随机种子 = 20250624；初值扰动半径 = 0.25 rad；"
        "位置阈值 = 5 mm；姿态阈值 = 3°。",
        ha="center",
        fontsize=6.3,
        color="#50555C",
    )

    fig.savefig(OUTPUT_STEM.with_suffix(".svg"), bbox_inches="tight")
    fig.savefig(OUTPUT_STEM.with_suffix(".pdf"), bbox_inches="tight")
    fig.savefig(OUTPUT_STEM.with_suffix(".png"), dpi=600, bbox_inches="tight")
    fig.savefig(OUTPUT_STEM.with_suffix(".tiff"), dpi=600, bbox_inches="tight")
    print(f"Wrote {OUTPUT_STEM}.[svg|pdf|png|tiff]")


if __name__ == "__main__":
    main()
