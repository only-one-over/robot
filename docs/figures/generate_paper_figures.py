from pathlib import Path
import warnings

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

warnings.filterwarnings("error", message=r"Glyph .* missing from current font\.")

OUT_DIR = Path(__file__).resolve().parent
CJK_FONT = Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
if CJK_FONT.exists():
    font_manager.fontManager.addfont(str(CJK_FONT))
    CJK_FAMILY = font_manager.FontProperties(fname=str(CJK_FONT)).get_name()
else:
    CJK_FAMILY = "Noto Sans CJK SC"

mpl.rcParams.update({
    "font.family": "sans-serif",
    "font.sans-serif": [CJK_FAMILY, "Microsoft YaHei", "SimHei", "DejaVu Sans", "sans-serif"],
    "svg.fonttype": "none",
    "pdf.fonttype": 42,
    "font.size": 7,
    "axes.spines.right": False,
    "axes.spines.top": False,
    "axes.linewidth": 0.8,
    "legend.frameon": False,
})

COLORS = {
    "ink": "#1f2933",
    "muted": "#667085",
    "blue": "#3b6ea8",
    "blue_light": "#dbe9f6",
    "green": "#3f7f5f",
    "green_light": "#dff0e7",
    "gold": "#9a6b20",
    "gold_light": "#f3e7cf",
    "gray": "#f2f4f7",
    "line": "#667085",
    "red": "#9b2c2c",
    "red_light": "#f4dddd",
}


def setup_ax(width=7.0, height=4.2):
    fig, ax = plt.subplots(figsize=(width, height), dpi=180)
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    fig.patch.set_facecolor("white")
    ax.set_facecolor("white")
    return fig, ax


def box(ax, xy, wh, text, fill, edge=None, fs=8, weight="normal"):
    x, y = xy
    w, h = wh
    patch = FancyBboxPatch(
        (x, y),
        w,
        h,
        boxstyle="round,pad=0.012,rounding_size=0.01",
        linewidth=0.9,
        edgecolor=edge or COLORS["line"],
        facecolor=fill,
    )
    ax.add_patch(patch)
    ax.text(
        x + w / 2,
        y + h / 2,
        text,
        ha="center",
        va="center",
        color=COLORS["ink"],
        fontsize=fs,
        fontweight=weight,
        linespacing=1.2,
    )
    return patch


def arrow(ax, start, end, color=None, rad=0.0, text=None, text_pos=0.5):
    arr = FancyArrowPatch(
        start,
        end,
        arrowstyle="-|>",
        mutation_scale=10,
        linewidth=1.05,
        color=color or COLORS["line"],
        connectionstyle=f"arc3,rad={rad}",
    )
    ax.add_patch(arr)
    if text:
        x = start[0] + (end[0] - start[0]) * text_pos
        y = start[1] + (end[1] - start[1]) * text_pos
        ax.text(x, y, text, ha="center", va="center", fontsize=6.5, color=COLORS["muted"])


def panel_label(ax, label):
    ax.text(0.015, 0.965, label, fontsize=9, fontweight="bold", va="top", color=COLORS["ink"])


def save(fig, stem):
    for ext in ("svg", "pdf", "png"):
        kwargs = {"bbox_inches": "tight"}
        if ext == "png":
            kwargs["dpi"] = 600
        fig.savefig(OUT_DIR / f"{stem}.{ext}", **kwargs)
    plt.close(fig)


def figure_system_architecture():
    fig, ax = setup_ax(7.0, 3.6)
    panel_label(ax, "a")
    ax.text(
        0.5,
        0.96,
        "理论—仿真一体化验证架构",
        ha="center",
        va="top",
        fontsize=10,
        fontweight="bold",
        color=COLORS["ink"],
    )

    box(ax, (0.06, 0.70), (0.22, 0.13), "运动学假设\nMDH / URDF", COLORS["blue_light"], COLORS["blue"], 8, "bold")
    box(ax, (0.39, 0.70), (0.22, 0.13), "位姿表示\n矩阵 + 对偶四元数", COLORS["gold_light"], COLORS["gold"], 8, "bold")
    box(ax, (0.72, 0.70), (0.22, 0.13), "数值逆运动学\nDLS + 关节限位", COLORS["green_light"], COLORS["green"], 8, "bold")

    box(ax, (0.12, 0.40), (0.23, 0.12), "轨迹参数化\n五次关节轨迹", COLORS["gray"], COLORS["line"], 7.5)
    box(ax, (0.385, 0.40), (0.23, 0.12), "控制器接口\nFollowJointTrajectory", COLORS["gray"], COLORS["line"], 7.5)
    box(ax, (0.65, 0.40), (0.23, 0.12), "仿真执行\nGazebo + ros2_control", COLORS["gray"], COLORS["line"], 7.5)

    box(ax, (0.20, 0.13), (0.24, 0.12), "状态反馈\njoint_states / TF", COLORS["red_light"], COLORS["red"], 7.5)
    box(ax, (0.56, 0.13), (0.24, 0.12), "误差核验\nFK 位姿残差", COLORS["red_light"], COLORS["red"], 7.5)

    arrow(ax, (0.28, 0.765), (0.39, 0.765))
    arrow(ax, (0.61, 0.765), (0.72, 0.765))
    arrow(ax, (0.83, 0.70), (0.76, 0.52))
    arrow(ax, (0.65, 0.46), (0.615, 0.46))
    arrow(ax, (0.385, 0.46), (0.35, 0.46))
    arrow(ax, (0.24, 0.40), (0.30, 0.25))
    arrow(ax, (0.77, 0.40), (0.68, 0.25))
    arrow(ax, (0.44, 0.19), (0.56, 0.19), color=COLORS["red"])
    arrow(ax, (0.68, 0.25), (0.58, 0.70), color=COLORS["red"], rad=0.35)

    ax.text(
        0.5,
        0.045,
        "核心结论：有效的运动学模型必须在数学表示、控制器接口与仿真状态反馈之间保持一致。",
        ha="center",
        va="bottom",
        fontsize=6.8,
        color=COLORS["muted"],
    )
    save(fig, "fig1_system_architecture")


def figure_kinematics_flow():
    fig, ax = setup_ax(7.0, 3.4)
    panel_label(ax, "b")
    ax.text(
        0.5,
        0.96,
        "齐次矩阵与对偶四元数运动学计算流程",
        ha="center",
        va="top",
        fontsize=10,
        fontweight="bold",
        color=COLORS["ink"],
    )

    box(ax, (0.06, 0.69), (0.21, 0.13), "URDF 局部关节链\n原点 + 旋转轴", COLORS["blue_light"], COLORS["blue"], 8, "bold")
    box(ax, (0.39, 0.69), (0.22, 0.13), "等效 MDH 变换\n$T_i(q_i)$", COLORS["blue_light"], COLORS["blue"], 8, "bold")
    box(ax, (0.73, 0.69), (0.21, 0.13), "FK 矩阵\n$T=\\prod_i T_i$", COLORS["blue_light"], COLORS["blue"], 8, "bold")

    box(ax, (0.20, 0.40), (0.24, 0.13), "对偶四元数转换\n$Q_i=q_r+\\varepsilon q_d$", COLORS["gold_light"], COLORS["gold"], 8, "bold")
    box(ax, (0.56, 0.40), (0.24, 0.13), "对偶四元数 FK\n$Q=Q_1\\otimes\\cdots\\otimes Q_n$", COLORS["gold_light"], COLORS["gold"], 8, "bold")

    box(ax, (0.20, 0.13), (0.24, 0.12), "位姿残差\n位置误差 + 姿态误差", COLORS["green_light"], COLORS["green"], 7.5)
    box(ax, (0.56, 0.13), (0.24, 0.12), "DLS 更新\n$\\Delta q=-J^T(JJ^T+\\lambda^2I)^{-1}e$", COLORS["green_light"], COLORS["green"], 6.4)

    arrow(ax, (0.27, 0.755), (0.39, 0.755))
    arrow(ax, (0.61, 0.755), (0.73, 0.755))
    arrow(ax, (0.50, 0.69), (0.32, 0.53))
    arrow(ax, (0.44, 0.465), (0.56, 0.465))
    arrow(ax, (0.835, 0.69), (0.68, 0.53))
    arrow(ax, (0.68, 0.40), (0.68, 0.25))
    arrow(ax, (0.44, 0.19), (0.56, 0.19))
    arrow(ax, (0.32, 0.40), (0.32, 0.25))

    ax.text(
        0.5,
        0.045,
        "核心结论：对偶四元数是等价的 SE(3) 表示，URDF 对齐的 MDH 链仍是建模依据。",
        ha="center",
        va="bottom",
        fontsize=6.8,
        color=COLORS["muted"],
    )
    save(fig, "fig2_kinematics_flow")


def figure_validation_loop():
    fig, ax = setup_ax(7.0, 3.6)
    panel_label(ax, "c")
    ax.text(
        0.5,
        0.96,
        "位姿指令的闭环验证流程",
        ha="center",
        va="top",
        fontsize=10,
        fontweight="bold",
        color=COLORS["ink"],
    )

    centers = [
        (0.17, 0.73, "目标位姿\nT* / Q*"),
        (0.43, 0.73, "IK 求解器\n多初值 DLS"),
        (0.69, 0.73, "关节轨迹\n五次 q(t)"),
        (0.82, 0.43, "控制器\n轨迹动作"),
        (0.56, 0.22, "仿真\n关节执行"),
        (0.29, 0.22, "状态反馈\njoint_states"),
        (0.12, 0.43, "FK 核验\n位姿残差"),
    ]
    for x, y, label in centers:
        if "目标" in label:
            fill, edge = COLORS["blue_light"], COLORS["blue"]
        elif "IK" in label or "FK" in label:
            fill, edge = COLORS["green_light"], COLORS["green"]
        elif "状态" in label or "仿真" in label:
            fill, edge = COLORS["red_light"], COLORS["red"]
        else:
            fill, edge = COLORS["gray"], COLORS["line"]
        box(ax, (x - 0.095, y - 0.055), (0.19, 0.11), label, fill, edge, 7.6, "bold" if "IK" in label else "normal")

    arrow(ax, (0.265, 0.73), (0.335, 0.73))
    arrow(ax, (0.525, 0.73), (0.595, 0.73))
    arrow(ax, (0.75, 0.68), (0.795, 0.485))
    arrow(ax, (0.73, 0.385), (0.64, 0.275))
    arrow(ax, (0.47, 0.22), (0.385, 0.22))
    arrow(ax, (0.205, 0.275), (0.145, 0.385))
    arrow(ax, (0.12, 0.485), (0.17, 0.675), color=COLORS["red"], rad=0.18)

    box(ax, (0.37, 0.43), (0.26, 0.11), "失败门控\n目标不可达 → 不生成轨迹", COLORS["gold_light"], COLORS["gold"], 7.4)
    arrow(ax, (0.43, 0.675), (0.47, 0.54), color=COLORS["gold"])
    arrow(ax, (0.63, 0.51), (0.69, 0.675), color=COLORS["gold"], rad=-0.12)

    ax.text(
        0.5,
        0.045,
        "核心结论：验证闭环同时检查数值收敛性与收敛解在仿真关节系统中的可执行性。",
        ha="center",
        va="bottom",
        fontsize=6.8,
        color=COLORS["muted"],
    )
    save(fig, "fig3_validation_loop")


def main():
    figure_system_architecture()
    figure_kinematics_flow()
    figure_validation_loop()


if __name__ == "__main__":
    main()
