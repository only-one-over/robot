# Course Report Outline

## 1. Project Goal

Build a ROS 2 Jazzy and Gazebo simulation for a simplified six-axis robot arm.
The system models the arm, computes forward and inverse kinematics, generates a
smooth joint trajectory, and drives the simulated robot through
`joint_trajectory_controller`.

## 2. Model Parameters

The model is inspired by the textbook repository's six-axis arm parameters:

| Parameter | Value |
| --- | ---: |
| `d1` | `0.0985 m` |
| `a2` | `-0.408 m` |
| `a3` | `-0.376 m` |
| `d4` | `0.1215 m` |
| `d5` | `0.1025 m` |
| `d6` | `0.094 m` |

The URDF/Xacro and the C++ FK function use the same joint origins and axes:

| Joint | Axis | Role |
| --- | --- | --- |
| `joint_1` | `z` | base yaw |
| `joint_2` | `-y` | shoulder pitch |
| `joint_3` | `-y` | elbow pitch |
| `joint_4` | `-y` | wrist pitch |
| `joint_5` | `-z` | wrist yaw |
| `joint_6` | `-y` | tool pitch |

## 3. ROS 2 Architecture

- `robot_arm_description`: Xacro/URDF model and RViz config.
- `robot_arm_kinematics`: FK, IK, trajectory generation, `/target_pose` node.
- `robot_arm_bringup`: Gazebo world, controller YAML, simulation launch.

Runtime data flow:

1. `move_to_pose_demo` publishes a reachable `/target_pose`.
2. `kinematics_node` solves IK from the current `/joint_states`.
3. `kinematics_node` publishes `/fk_pose` and `/ik_status`.
4. `kinematics_node` sends a `FollowJointTrajectory` action goal.
5. `joint_trajectory_controller` moves the Gazebo robot.

## 4. Algorithms

- FK multiplies the same homogeneous transforms used by the Xacro joint chain.
- IK uses damped least squares with a finite-difference Jacobian and multiple
  initial seeds.
- Trajectory generation uses a quintic blend:
  `s(t) = 10t^3 - 15t^4 + 6t^5`.

## 5. Experiments

Run:

```bash
colcon build --symlink-install
source install/setup.bash
ros2 run robot_arm_kinematics fk_ik_test
ros2 launch robot_arm_bringup sim.launch.py
ros2 run robot_arm_kinematics move_to_pose_demo
```

Suggested screenshots:

- RViz or Gazebo initial model.
- Robot moving to the first target.
- `/ik_status` output showing convergence and error.

Acceptance criteria:

- FK/IK console test passes.
- Position error is below `5 mm`.
- Orientation error is below `3 deg`.
- Gazebo robot follows all demo targets without controller errors.

