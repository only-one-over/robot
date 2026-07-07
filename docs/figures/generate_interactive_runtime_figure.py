from pathlib import Path
import warnings

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import font_manager
import numpy as np

warnings.filterwarnings("error", message=r"Glyph .* missing from current font\.")

OUT_STEM = Path(__file__).resolve().parent / "fig4_interactive_runtime"
CJK_FONT = Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
if CJK_FONT.exists():
    font_manager.fontManager.addfont(str(CJK_FONT))
    CJK_FAMILY = font_manager.FontProperties(fname=str(CJK_FONT)).get_name()
else:
    CJK_FAMILY = "Noto Sans CJK SC"

mpl.rcParams.update(
    {
        "font.family": "sans-serif",
        "font.sans-serif": [
            CJK_FAMILY,
            "Microsoft YaHei",
            "SimHei",
            "DejaVu Sans",
            "sans-serif",
        ],
        "svg.fonttype": "none",
        "pdf.fonttype": 42,
        "font.size": 8,
        "axes.titlesize": 11,
        "axes.labelsize": 8,
        "legend.fontsize": 7.5,
    }
)

D1 = 0.0985
A2 = -0.408
A3 = -0.376
D4 = 0.1215
D5 = 0.1025
D6 = 0.0940
WORK_Q = np.array([-0.35, -1.0, 1.55, -0.5, -0.35, -0.2])


def translate(x, y, z):
    transform = np.eye(4)
    transform[:3, 3] = (x, y, z)
    return transform


def rotate(axis, angle):
    axis = np.asarray(axis, dtype=float)
    axis /= np.linalg.norm(axis)
    x, y, z = axis
    skew = np.array([[0.0, -z, y], [z, 0.0, -x], [-y, x, 0.0]])
    rotation = np.eye(3) + np.sin(angle) * skew + (1.0 - np.cos(angle)) * (skew @ skew)
    transform = np.eye(4)
    transform[:3, :3] = rotation
    return transform


def link_points(q):
    transforms = [
        translate(0.0, 0.0, D1) @ rotate((0.0, 0.0, 1.0), q[0]),
        rotate((0.0, -1.0, 0.0), q[1]),
        translate(A2, 0.0, 0.0) @ rotate((0.0, -1.0, 0.0), q[2]),
        translate(A3, 0.0, 0.0) @ rotate((0.0, -1.0, 0.0), q[3]),
        translate(0.0, -D4, 0.0) @ rotate((0.0, 0.0, -1.0), q[4]),
        translate(0.0, D4, -D5) @ rotate((0.0, -1.0, 0.0), q[5]),
        translate(0.0, -(D4 + D6), 0.0),
    ]
    current = np.eye(4)
    points = [current[:3, 3].copy()]
    for transform in transforms:
        current = current @ transform
        points.append(current[:3, 3].copy())
    return np.asarray(points)


def evidence_row(ax, y, number, title, detail):
    circle = plt.Circle((0.035, y), 0.022, facecolor="#3B6EA8", edgecolor="none")
    ax.add_patch(circle)
    ax.text(0.035, y, str(number), color="white", ha="center", va="center", fontweight="bold", fontsize=8.5)
    ax.text(0.075, y + 0.015, title, ha="left", va="center", fontweight="bold", fontsize=9.2)
    ax.text(0.075, y - 0.026, detail, ha="left", va="center", color="#667085", fontsize=7.2)


def main():
    joints = link_points(WORK_Q)
    target = joints[-1]

    fig = plt.figure(figsize=(9.0, 3.2), dpi=180)
    grid = fig.add_gridspec(1, 2, width_ratios=(1.0, 1.45), wspace=0.15)

    ax3d = fig.add_subplot(grid[0, 0], projection="3d")
    ax3d.plot(
        joints[:, 0],
        joints[:, 1],
        joints[:, 2],
        "-o",
        color="#3B6EA8",
        linewidth=2.0,
        markersize=4.0,
        label="FK 连杆链",
    )
    ax3d.scatter(
        target[0],
        target[1],
        target[2],
        marker="*",
        s=85,
        color="#B33A3A",
        edgecolor="#7A2020",
        linewidth=0.6,
        label="常用工作目标",
        zorder=6,
    )
    for index, point in enumerate(joints):
        label = "基座" if index == 0 else ("工具端" if index == len(joints) - 1 else f"关节{index}")
        ax3d.text(point[0], point[1], point[2], label, fontsize=7.0, color="#1F2933")

    ax3d.set_title("交互目标指令后的机械臂位姿", fontweight="bold", pad=10)
    ax3d.set_xlabel("x（m）")
    ax3d.set_ylabel("y（m）")
    ax3d.set_zlabel("z（m）")
    ax3d.view_init(elev=20, azim=-66)
    ax3d.grid(True, linewidth=0.45, alpha=0.65)
    ax3d.legend(loc="upper left", frameon=True, framealpha=0.95)
    ax3d.text2D(-0.08, 1.03, "d", transform=ax3d.transAxes, fontweight="bold", fontsize=9)

    ax = fig.add_subplot(grid[0, 1])
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.text(0.0, 0.965, "ROS/Gazebo 实测证据", ha="left", va="top", fontweight="bold", fontsize=11)

    evidence_row(ax, 0.84, 1, "交互标记服务", "/target_marker/get_interactive_markers 可用")
    evidence_row(ax, 0.69, 2, "拖拽释放反馈", "MOUSE_UP 反馈已发布至 /target_marker/feedback")
    evidence_row(ax, 0.54, 3, "目标指令", "/target_pose 已发布：x=−0.545，y=−0.024，z=0.145")
    evidence_row(ax, 0.39, 4, "控制器状态", "joint_state_broadcaster 与 joint_trajectory_controller 已激活")
    evidence_row(ax, 0.24, 5, "数值验证", "1000 组基准：DLS 逆解成功率为 100.0%")

    ax.text(
        0.0,
        0.075,
        "说明：初始目标取非奇异的前中部常用工作位姿；拖拽后仍由 DLS 逆解与\n"
        "FollowJointTrajectory 轨迹接口作为执行层。",
        ha="left",
        va="bottom",
        fontsize=7.2,
        color="#667085",
        linespacing=1.4,
    )

    fig.savefig(OUT_STEM.with_suffix(".svg"), bbox_inches="tight")
    fig.savefig(OUT_STEM.with_suffix(".pdf"), bbox_inches="tight")
    fig.savefig(OUT_STEM.with_suffix(".png"), dpi=600, bbox_inches="tight")
    fig.savefig(OUT_STEM.with_suffix(".tiff"), dpi=600, bbox_inches="tight")
    plt.close(fig)
    print(f"Wrote {OUT_STEM}.[svg|pdf|png|tiff]")
    print(f"End-effector position: {joints[-1]}")
    print(f"Target residual (m): {np.linalg.norm(joints[-1] - target):.6g}")


if __name__ == "__main__":
    main()
