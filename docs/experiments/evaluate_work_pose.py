"""Evaluate candidate work configurations against the project kinematic model."""

from __future__ import annotations

import numpy as np


D1 = 0.0985
A2 = -0.408
A3 = -0.376
D4 = 0.1215
D5 = 0.1025
D6 = 0.0940
LOWER = np.array([-np.pi, -2.61799, -2.61799, -np.pi, -np.pi, -np.pi])
UPPER = np.array([np.pi, 2.61799, 2.61799, np.pi, np.pi, np.pi])


def translate(x: float, y: float, z: float) -> np.ndarray:
    transform = np.eye(4)
    transform[:3, 3] = (x, y, z)
    return transform


def rotate(axis: tuple[float, float, float], angle: float) -> np.ndarray:
    vector = np.asarray(axis, dtype=float)
    vector /= np.linalg.norm(vector)
    x, y, z = vector
    skew = np.array([[0.0, -z, y], [z, 0.0, -x], [-y, x, 0.0]])
    rotation = np.eye(3) + np.sin(angle) * skew + (1.0 - np.cos(angle)) * (skew @ skew)
    transform = np.eye(4)
    transform[:3, :3] = rotation
    return transform


def forward(q: np.ndarray) -> np.ndarray:
    transform = np.eye(4)
    local_transforms = (
        translate(0.0, 0.0, D1) @ rotate((0.0, 0.0, 1.0), q[0]),
        rotate((0.0, -1.0, 0.0), q[1]),
        translate(A2, 0.0, 0.0) @ rotate((0.0, -1.0, 0.0), q[2]),
        translate(A3, 0.0, 0.0) @ rotate((0.0, -1.0, 0.0), q[3]),
        translate(0.0, -D4, 0.0) @ rotate((0.0, 0.0, -1.0), q[4]),
        translate(0.0, D4, -D5) @ rotate((0.0, -1.0, 0.0), q[5]),
        translate(0.0, -(D4 + D6), 0.0),
    )
    for local in local_transforms:
        transform = transform @ local
    return transform


def pose_error(target: np.ndarray, actual: np.ndarray) -> np.ndarray:
    error = np.empty(6)
    error[:3] = target[:3, 3] - actual[:3, 3]
    target_rotation = target[:3, :3]
    actual_rotation = actual[:3, :3]
    error[3:] = 0.5 * sum(
        np.cross(actual_rotation[:, index], target_rotation[:, index])
        for index in range(3)
    )
    return error


def error_jacobian(q: np.ndarray, step: float = 1.0e-6) -> np.ndarray:
    target = forward(q)
    jacobian = np.empty((6, 6))
    for column in range(6):
        q_minus = q.copy()
        q_plus = q.copy()
        q_minus[column] -= step
        q_plus[column] += step
        jacobian[:, column] = (
            pose_error(target, forward(q_plus)) -
            pose_error(target, forward(q_minus))
        ) / (2.0 * step)
    return jacobian


def report(name: str, values: list[float]) -> None:
    q = np.asarray(values, dtype=float)
    transform = forward(q)
    singular_values = np.linalg.svd(error_jacobian(q), compute_uv=False)
    condition = singular_values[0] / singular_values[-1]
    joint_margin = np.min(np.minimum(q - LOWER, UPPER - q))
    position = transform[:3, 3]
    radial_distance = np.linalg.norm(position[:2])
    print(
        f"{name}: "
        f"q={np.array2string(q, precision=3)}, "
        f"p={np.array2string(position, precision=6)}, "
        f"radial={radial_distance:.4f} m, "
        f"z={position[2]:.4f} m, "
        f"limit_margin={joint_margin:.4f} rad, "
        f"sigma_min={singular_values[-1]:.6f}, "
        f"condition={condition:.2f}"
    )


def main() -> None:
    candidates = {
        "current_marker": [0.0, -0.8, 1.4, -0.6, 0.0, 0.0],
        "demo_q1": [0.25, -0.7, 1.25, -0.45, 0.3, 0.2],
        "demo_q2": [-0.35, -1.0, 1.55, -0.5, -0.35, -0.2],
        "demo_q3": [0.5, -0.75, 1.3, -0.5, 0.25, 0.35],
        "front_center": [0.2, -0.85, 1.35, -0.45, 0.25, 0.15],
    }
    for name, values in candidates.items():
        report(name, values)


if __name__ == "__main__":
    main()
