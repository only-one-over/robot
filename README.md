# ROS 2 Jazzy Six-Axis Robot Arm Course Project

This workspace implements a simplified six-axis robot arm inspired by the
`ch3/kinematics` material in
`mhuasong/Basics-of-Robotics-Theory-and-Technology`.

The project targets ROS 2 Jazzy on Ubuntu 24.04 with modern Gazebo (`ros_gz`)
and `ros2_control`.

## Packages

- `robot_arm_description`: Xacro/URDF model, RViz config, ros2_control tags.
- `robot_arm_kinematics`: Eigen-based FK, numerical IK, trajectory generation,
  and ROS 2 nodes.
- `robot_arm_bringup`: Gazebo world, controller configuration, launch files.

## Build

```bash
colcon build --symlink-install
source install/setup.bash
```

## Run Simulation

```bash
ros2 launch robot_arm_bringup sim.launch.py
```

In another terminal:

```bash
source install/setup.bash
ros2 run robot_arm_kinematics move_to_pose_demo
```

## Run FK/IK Console Test

```bash
source install/setup.bash
ros2 run robot_arm_kinematics fk_ik_test
```

## Main Topics

- Subscribe: `/target_pose` (`geometry_msgs/msg/PoseStamped`)
- Publish: `/fk_pose` (`geometry_msgs/msg/PoseStamped`)
- Publish: `/ik_status` (`std_msgs/msg/String`)
- Action client:
  `/joint_trajectory_controller/follow_joint_trajectory`

