# ROS 2 Jazzy 六自由度机械臂运动学建模与仿真验证

本工作空间实现了一个简化的六自由度机械臂，其运动学设计参考
`mhuasong/Basics-of-Robotics-Theory-and-Technology` 仓库中
`ch3/kinematics` 的相关内容。

项目面向 Ubuntu 24.04 与 ROS 2 Jazzy，使用新版 Gazebo（`ros_gz`）
完成物理仿真，并通过 `ros2_control` 驱动机械臂关节。

完整的理论模型、公式推导、实验设计与结果分析详见
[`docs/robot_control_technology_research_report.docx`](docs/robot_control_technology_research_report.docx)。

## 功能包结构

- `robot_arm_description`：Xacro/URDF 机械臂模型、RViz 配置及
  `ros2_control` 描述。
- `robot_arm_kinematics`：基于 Eigen 的正运动学、数值逆运动学、
  轨迹生成算法和 ROS 2 节点。
- `robot_arm_bringup`：Gazebo 世界、控制器配置和启动文件。

## 编译项目

在 ROS 2 工作空间根目录执行：

```bash
colcon build --symlink-install
source install/setup.bash
```

## 启动 Gazebo 仿真

```bash
ros2 launch robot_arm_bringup sim.launch.py
```

在另一个终端中加载工作空间并运行预设目标位姿实验：

```bash
source install/setup.bash
ros2 run robot_arm_kinematics move_to_pose_demo
```

## 使用 RViz 拖动控制

以下命令会同时启动 Gazebo、目标位姿交互标记节点和 RViz：

```bash
ros2 launch robot_arm_bringup interactive_control.launch.py
```

机械臂以关节角 `[-0.35, -1.0, 1.55, -0.5, -0.35, -0.2] rad`
作为前中部常用工作构型。该构型已避开关节限位与腕部奇异状态，并与 Gazebo
中的初始关节状态及 RViz 交互目标一致。

在 RViz 工具栏中选择 `Interact` 工具，然后拖动 `tool0` 目标标记。
松开鼠标后，交互标记节点会向 `/target_pose` 发布
`geometry_msgs/msg/PoseStamped` 消息。运动学节点接收目标位姿并求解逆运动学，
随后将生成的关节轨迹发送给 `joint_trajectory_controller`，驱动 Gazebo
中的机械臂运动。

如果 Gazebo 已经启动，也可以单独运行交互标记节点：

```bash
ros2 run robot_arm_kinematics target_marker_node
```

## 运行正逆运动学测试

```bash
source install/setup.bash
ros2 run robot_arm_kinematics fk_ik_test
```

运行 1000 组随机样本的三种逆运动学算法对比实验：

```bash
ros2 run robot_arm_kinematics fk_ik_benchmark --samples 1000 --seed 20250624
```

该实验在相同初值扰动、关节限位和收敛阈值下，对比雅可比转置法、
Moore-Penrose 伪逆法和阻尼最小二乘法的成功率、位姿误差、迭代次数与运行时间。

## 主要 ROS 2 接口

- 订阅目标位姿：`/target_pose`（`geometry_msgs/msg/PoseStamped`）
- 发布正运动学位姿：`/fk_pose`（`geometry_msgs/msg/PoseStamped`）
- 发布逆运动学状态：`/ik_status`（`std_msgs/msg/String`）
- 交互标记更新：`/target_marker/update`
- 关节轨迹动作接口：
  `/joint_trajectory_controller/follow_joint_trajectory`
