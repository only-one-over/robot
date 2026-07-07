# Code Wiki — ROS 2 Jazzy Six-Axis Robot Arm

## 1. 项目概述

本项目是一个基于 **ROS 2 Jazzy** 的六轴机械臂仿真课程项目，灵感来源于 `mhuasong/Basics-of-Robotics-Theory-and-Technology` 教材第 3 章运动学部分。项目在 **Ubuntu 24.04** 平台上运行，使用现代 **Gazebo** (`ros_gz`) 仿真器和 **ros2_control** 框架，实现了机械臂建模、正/逆运动学求解、轨迹规划与仿真控制等完整功能链路。

**核心能力：**
- 基于 Xacro/URDF 的六轴机械臂参数化建模
- 基于 Eigen 的正向运动学 (FK) 与阻尼最小二乘逆运动学 (IK)
- 五次多项式轨迹插值生成
- 通过 `ros2_control` + `joint_trajectory_controller` 驱动 Gazebo 仿真

---

## 2. 项目目录结构

```
robot/
├── README.md                              # 项目说明
├── .gitignore                             # Git 忽略规则
├── docs/
│   └── course_report_outline.md           # 课程报告大纲
└── src/
    ├── robot_arm_description/             # [包] 机器人模型描述
    │   ├── CMakeLists.txt
    │   ├── package.xml
    │   ├── urdf/
    │   │   └── six_axis_arm.urdf.xacro    # 核心：机械臂 URDF/Xacro 模型
    │   ├── launch/
    │   │   └── display.launch.py          # RViz 可视化启动文件
    │   └── rviz/
    │       └── six_axis_arm.rviz          # RViz 配置
    ├── robot_arm_kinematics/              # [包] 运动学算法与 ROS 节点
    │   ├── CMakeLists.txt
    │   ├── package.xml
    │   ├── include/robot_arm_kinematics/
    │   │   └── kinematics.hpp             # 核心：运动学类接口定义
    │   └── src/
    │       ├── kinematics.cpp             # 核心：FK/IK/轨迹算法实现
    │       ├── kinematics_node.cpp        # 核心：运动学 ROS 控制节点
    │       ├── move_to_pose_demo.cpp      # 演示：目标位姿发布节点
    │       └── fk_ik_test.cpp             # 测试：FK/IK 控制台验证
    └── robot_arm_bringup/                 # [包] 仿真启动与控制器配置
        ├── CMakeLists.txt
        ├── package.xml
        ├── launch/
        │   └── sim.launch.py              # 核心：仿真总启动文件
        ├── config/
        │   └── controllers.yaml           # 核心：控制器参数配置
        └── worlds/
            └── empty.sdf                  # Gazebo 空世界模型
```

---

## 3. 整体架构

### 3.1 系统分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                     应用层 (Application)                     │
│  move_to_pose_demo ──► /target_pose ──► kinematics_node     │
└──────────────────────────────┬──────────────────────────────┘
                               │ FollowJointTrajectory Action
┌──────────────────────────────▼──────────────────────────────┐
│                   控制层 (ros2_control)                      │
│  joint_trajectory_controller ◄── joint_state_broadcaster    │
└──────────────────────────────┬──────────────────────────────┘
                               │ position command / state
┌──────────────────────────────▼──────────────────────────────┐
│                   仿真层 (Gazebo Sim)                        │
│  gz_ros2_control::GazeboSimROS2ControlPlugin                │
│  GazeboSimSystem (hardware interface)                       │
└──────────────────────────────┬──────────────────────────────┘
                               │ URDF/SDF
┌──────────────────────────────▼──────────────────────────────┐
│                   模型层 (Description)                       │
│  six_axis_arm.urdf.xacro (links, joints, materials)         │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 运行时数据流

```
move_to_pose_demo                 kinematics_node              Gazebo + ros2_control
      │                                │                              │
      │  /target_pose                  │                              │
      │ (PoseStamped)                  │                              │
      ├───────────────────────────────►│                              │
      │                                │◄── /joint_states ───────────┤
      │                                │    (JointState)              │
      │                                │                              │
      │                                │── FK ──► /fk_pose           │
      │                                │           (PoseStamped)      │
      │                                │                              │
      │                                │── IK status ► /ik_status    │
      │                                │           (String)           │
      │                                │                              │
      │                                │── FollowJointTrajectory ────►│
      │                                │   (Action Goal)              │
      │                                │                              │
      │                                │◄── Action Result ───────────┤
      │                                │                              │
```

**数据流步骤：**
1. `move_to_pose_demo` 通过 FK 计算目标关节角对应的末端位姿，发布到 `/target_pose`
2. `kinematics_node` 订阅 `/target_pose`，结合当前 `/joint_states` 执行 IK 求解
3. IK 收敛后，`kinematics_node` 生成五次多项式轨迹，通过 Action 发送给 `joint_trajectory_controller`
4. 控制器驱动 Gazebo 中的机械臂执行轨迹

---

## 4. 包详解

### 4.1 robot_arm_description — 机器人模型描述

**职责：** 定义机械臂的物理结构、视觉外观、碰撞几何和 ros2_control 硬件接口。

#### 关键文件

| 文件 | 说明 |
|------|------|
| `urdf/six_axis_arm.urdf.xacro` | 机械臂完整 URDF 模型（含 link/joint/inertial/visual/collision/ros2_control） |
| `launch/display.launch.py` | RViz 可视化启动（robot_state_publisher + joint_state_publisher_gui + rviz2） |
| `rviz/six_axis_arm.rviz` | RViz 显示配置 |

#### 机械臂连杆与关节

**DH 参数：**

| 参数 | 值 | 说明 |
|------|-----|------|
| `d1` | 0.0985 m | 基座到关节1的Z偏移 |
| `a2` | -0.408 m | 关节2到关节3的X偏移 |
| `a3` | -0.376 m | 关节3到关节4的X偏移 |
| `d4` | 0.1215 m | 关节4到关节5的Y偏移 |
| `d5` | 0.1025 m | 关节5到关节6的Z偏移 |
| `d6` | 0.094 m | 末端工具偏移 |

**连杆结构：**

| 连杆 | 质量 (kg) | 几何形状 | 材质颜色 |
|------|-----------|----------|----------|
| `base_link` | 3.0 | 圆柱 (r=0.13, h=0.07) | 深灰 (0.08, 0.10, 0.12) |
| `link_1` | 1.5 | 圆柱 (r=0.055, h=d1) | 蓝色 (0.12, 0.42, 0.78) |
| `link_2` | 1.5 | 圆柱 (r=0.04, h=0.408) | 灰色 (0.62, 0.66, 0.70) |
| `link_3` | 1.2 | 圆柱 (r=0.035, h=0.376) | 蓝色 |
| `link_4` | 0.9 | 圆柱 (r=0.032, h=d4) | 灰色 |
| `link_5` | 0.8 | 圆柱 (r=0.03, h=d45_len) | 蓝色 |
| `link_6` | 0.6 | 圆柱 (r=0.026, h=d4+d6) | 橙色 (0.95, 0.48, 0.12) |
| `tool0` | 0.1 | 球体 (r=0.035) | 橙色 |

**关节配置：**

| 关节 | 类型 | 父连杆 | 子连杆 | 旋转轴 | 关节限位 (rad) | 力矩 (Nm) | 速度 (rad/s) |
|------|------|--------|--------|--------|----------------|-----------|-------------|
| `joint_1` | revolute | base_link | link_1 | Z (0,0,1) | [-π, π] | 60 | 1.5 |
| `joint_2` | revolute | link_1 | link_2 | -Y (0,-1,0) | [-2.618, 2.618] | 60 | 1.5 |
| `joint_3` | revolute | link_2 | link_3 | -Y (0,-1,0) | [-2.618, 2.618] | 45 | 1.5 |
| `joint_4` | revolute | link_3 | link_4 | -Y (0,-1,0) | [-π, π] | 35 | 2.0 |
| `joint_5` | revolute | link_4 | link_5 | -Z (0,0,-1) | [-π, π] | 25 | 2.0 |
| `joint_6` | revolute | link_5 | link_6 | -Y (0,-1,0) | [-π, π] | 20 | 2.5 |
| `tool0_fixed` | fixed | link_6 | tool0 | — | — | — | — |

**ros2_control 硬件接口：**
- 硬件插件：`gz_ros2_control/GazeboSimSystem`
- 每个关节提供 `position` 命令接口和 `position`/`velocity` 状态接口
- Gazebo 插件：`gz_ros2_control::GazeboSimROS2ControlPlugin`

#### 依赖

| 依赖类型 | 包名 |
|----------|------|
| exec_depend | `joint_state_publisher_gui`, `gz_ros2_control`, `robot_state_publisher`, `rviz2`, `xacro` |

---

### 4.2 robot_arm_kinematics — 运动学算法与 ROS 节点

**职责：** 实现正向运动学、逆运动学、轨迹插值算法，并提供 ROS 2 节点进行运动控制。

#### 关键类与函数

##### `SixAxisArmKinematics` 类

定义于 `include/robot_arm_kinematics/kinematics.hpp`，实现于 `src/kinematics.cpp`。

```cpp
class SixAxisArmKinematics {
public:
  // 正向运动学：给定关节角，计算末端位姿 (4x4 齐次变换矩阵)
  Eigen::Matrix4d forward(const Vector6d & q) const;

  // 逆运动学：给定目标位姿和初始种子，求解关节角
  IkResult inverse(const Eigen::Matrix4d & target, const Vector6d & seed) const;

  // 五次多项式轨迹插值：在起始和目标关节角之间生成平滑轨迹
  std::vector<Vector6d> quinticTrajectory(
    const Vector6d & start, const Vector6d & goal, int point_count) const;

  // 关节名称、限位访问器
  const std::array<std::string, 6> & jointNames() const;
  const Vector6d & lowerLimits() const;
  const Vector6d & upperLimits() const;

  // 静态工具函数：位姿误差计算
  static Vector6d poseError(const Eigen::Matrix4d & target, const Eigen::Matrix4d & actual);
  static double orientationErrorNorm(const Eigen::Matrix4d & target, const Eigen::Matrix4d & actual);

private:
  Eigen::Matrix4d translate(double x, double y, double z) const;
  Eigen::Matrix4d rotate(const Eigen::Vector3d & axis, double angle) const;
  Vector6d clampToLimits(const Vector6d & q) const;
};
```

##### `IkResult` 结构体

```cpp
struct IkResult {
  bool converged;          // 是否收敛
  Vector6d joints;         // 求解得到的关节角
  int iterations;          // 迭代次数
  double position_error;   // 位置误差 (m)
  double orientation_error;// 姿态误差 (rad)
  std::string message;     // 状态消息
};
```

##### 类型别名

```cpp
using Vector6d = Eigen::Matrix<double, 6, 1>;   // 6维向量（关节角）
using Matrix6d = Eigen::Matrix<double, 6, 6>;   // 6x6矩阵（Jacobian等）
```

#### 算法详解

##### 正向运动学 (FK)

采用**齐次变换矩阵连乘**方法，按照 DH 参数依次构建各关节的平移和旋转变换：

```
T = T_base · Rz(q1) · Ry(-q2) · Ry(-q3) · Ry(-q4) · Rz(-q5) · Ry(-q5) · T_tool
```

具体变换链（对应 `kinematics.cpp:56-68`）：

| 步骤 | 变换 | 说明 |
|------|------|------|
| 1 | `translate(0, 0, d1) * rotate(Z, q0)` | 基座偏移 + 关节1旋转 |
| 2 | `rotate(-Y, q1)` | 关节2旋转 |
| 3 | `translate(a2, 0, 0) * rotate(-Y, q2)` | 连杆2偏移 + 关节3旋转 |
| 4 | `translate(a3, 0, 0) * rotate(-Y, q3)` | 连杆3偏移 + 关节4旋转 |
| 5 | `translate(0, -d4, 0) * rotate(-Z, q4)` | 连杆4偏移 + 关节5旋转 |
| 6 | `translate(0, d4, -d5) * rotate(-Y, q5)` | 连杆5偏移 + 关节6旋转 |
| 7 | `translate(0, -(d4+d6), 0)` | 末端工具偏移 |

##### 逆运动学 (IK)

采用**阻尼最小二乘法 (Damped Least Squares / Levenberg-Marquardt)**，核心参数：

| 参数 | 值 | 说明 |
|------|-----|------|
| `kMaxIterations` | 250 | 最大迭代次数 |
| `kPositionTolerance` | 0.005 m (5mm) | 位置收敛容差 |
| `kOrientationTolerance` | 3° (0.0524 rad) | 姿态收敛容差 |
| `kDamping` | 0.08 | 阻尼系数 λ |
| `kMaxStep` | 0.18 rad | 单步最大关节角增量 |
| `kFiniteDifferenceStep` | 1e-5 | 数值差分步长 |

**算法流程：**
1. 使用 **4 组初始种子** 求解（用户种子、零位、肘上构型、肘下构型），取最优解
2. 每次迭代通过**有限差分法**计算 Jacobian 矩阵
3. 使用阻尼最小二乘更新：`Δq = -Jᵀ(JJᵀ + λ²I)⁻¹ · e`
4. 限制单步增量不超过 `kMaxStep`
5. 将关节角钳位到限位范围内
6. 选择收敛且误差最小的候选解

##### 位姿误差计算

- **位置误差**：目标位置与实际位置的差向量
- **姿态误差**：使用旋转矩阵列向量叉积的半和（`poseError`），或相对旋转矩阵的迹反算角度（`orientationErrorNorm`）

##### 五次多项式轨迹插值

使用 **quintic blend** 函数实现零速起停的平滑轨迹：

```
s(t) = 10t³ - 15t⁴ + 6t⁵
q(t) = q_start + s(t) · (q_goal - q_start)
```

特性：`s(0) = 0`, `s(1) = 1`, `s'(0) = 0`, `s'(1) = 0`（起止速度为零）。

#### ROS 节点

##### `kinematics_node` — 运动学控制节点

文件：`src/kinematics_node.cpp`，节点名：`six_axis_arm_kinematics_node`

**订阅话题：**

| 话题 | 消息类型 | 说明 |
|------|----------|------|
| `/target_pose` | `geometry_msgs/msg/PoseStamped` | 目标末端位姿 |
| `/joint_states` | `sensor_msgs/msg/JointState` | 当前关节状态 |

**发布话题：**

| 话题 | 消息类型 | 说明 |
|------|----------|------|
| `/fk_pose` | `geometry_msgs/msg/PoseStamped` | IK 解算后的 FK 验证位姿 |
| `/ik_status` | `std_msgs/msg/String` | IK 求解状态（收敛/误差/迭代次数） |

**Action 客户端：**

| Action | 类型 | 说明 |
|--------|------|------|
| `/joint_trajectory_controller/follow_joint_trajectory` | `control_msgs/action/FollowJointTrajectory` | 发送关节轨迹目标 |

**工作流程：**
1. 收到 `/target_pose` 后，将 Pose 消息转换为 4x4 齐次矩阵
2. 以当前关节角为种子执行 IK 求解
3. 发布 IK 状态到 `/ik_status`
4. 对 IK 结果执行 FK 验证，发布到 `/fk_pose`
5. 若 IK 收敛且 Action 服务器可用，生成 50 点、5 秒时长的轨迹并发送

**辅助函数：**

| 函数 | 说明 |
|------|------|
| `poseMsgToMatrix()` | 将 ROS Pose 消息转换为 Eigen 4x4 矩阵 |
| `matrixToPoseMsg()` | 将 Eigen 4x4 矩阵转换为 ROS Pose 消息 |
| `vectorToString()` | 将 Vector6d 格式化为字符串 |

##### `move_to_pose_demo` — 目标位姿演示节点

文件：`src/move_to_pose_demo.cpp`，节点名：`move_to_pose_demo`

- 预定义 3 组目标关节角，通过 FK 计算对应末端位姿
- 每 8 秒循环发布下一个目标位姿到 `/target_pose`

**预定义目标：**

| 编号 | 关节角 (rad) |
|------|-------------|
| 1 | [0.2, 0.35, -0.55, 0.75, -0.35, 0.25] |
| 2 | [-0.35, 0.55, -0.45, 0.35, 0.45, -0.2] |
| 3 | [0.5, 0.25, -0.7, 0.85, 0.25, 0.35] |

##### `fk_ik_test` — FK/IK 控制台测试

文件：`src/fk_ik_test.cpp`

- 测试 1：对关节角 `[0, π/2, 0, π/2, 0, 0]` 执行 FK 并打印末端位姿
- 测试 2：对关节角 `[0.2, 0.35, -0.55, 0.75, -0.35, 0.25]` 执行 FK→IK→FK 验证闭环误差
- 验收标准：位置误差 < 5mm，姿态误差 < 3°，否则返回退出码 1

#### 构建目标

| 目标 | 类型 | 源文件 | 依赖 |
|------|------|--------|------|
| `robot_arm_kinematics` (库) | 共享库 | `kinematics.cpp` | Eigen3 |
| `kinematics_node` | 可执行 | `kinematics_node.cpp` | 库 + control_msgs, geometry_msgs, rclcpp, rclcpp_action, sensor_msgs, std_msgs, trajectory_msgs |
| `move_to_pose_demo` | 可执行 | `move_to_pose_demo.cpp` | 库 + geometry_msgs, rclcpp |
| `fk_ik_test` | 可执行 | `fk_ik_test.cpp` | 库 + rclcpp |

#### 依赖

| 依赖类型 | 包名 |
|----------|------|
| depend | `control_msgs`, `geometry_msgs`, `rclcpp`, `rclcpp_action`, `sensor_msgs`, `std_msgs`, `trajectory_msgs` |
| build/build_export/exec | `eigen` |

---

### 4.3 robot_arm_bringup — 仿真启动与控制器配置

**职责：** 提供 Gazebo 仿真环境、ros2_control 控制器配置和统一的启动文件。

#### 关键文件

| 文件 | 说明 |
|------|------|
| `launch/sim.launch.py` | 仿真总启动文件 |
| `config/controllers.yaml` | 控制器参数配置 |
| `worlds/empty.sdf` | Gazebo 空世界定义 |

#### sim.launch.py 启动流程

该文件按顺序启动以下节点和操作：

| 延迟 (s) | 启动项 | 说明 |
|-----------|--------|------|
| 0 | Gazebo (`ros_gz_sim/gz_sim.launch.py`) | 启动 Gazebo 仿真器，加载空世界 |
| 0 | `ros_gz_bridge/parameter_bridge` | 将 Gazebo `/clock` 单向桥接至 ROS 2，驱动仿真时间与动态 TF |
| 0 | `robot_state_publisher` | 发布机器人 TF 树（使用 sim_time） |
| 0 | `ros_gz_sim/create` | 在 Gazebo 中生成机器人模型 (z=0.02) |
| 3 | `joint_state_broadcaster` (spawner) | 启动关节状态广播器 |
| 4 | `joint_trajectory_controller` (spawner) | 启动关节轨迹控制器 |
| 5 | `kinematics_node` | 启动运动学控制节点 |

**Xacro 参数传递：** `controllers_file` 参数被传入 Xacro 文件，供 Gazebo ros2_control 插件加载控制器配置。

#### controllers.yaml 控制器配置

**控制器管理器：**
- 更新频率：100 Hz

**已配置控制器：**

| 控制器 | 类型 | 说明 |
|--------|------|------|
| `joint_state_broadcaster` | `joint_state_broadcaster/JointStateBroadcaster` | 广播关节状态到 `/joint_states` |
| `joint_trajectory_controller` | `joint_trajectory_controller/JointTrajectoryController` | 接收轨迹目标并控制关节运动 |

**轨迹控制器参数：**

| 参数 | 值 | 说明 |
|------|-----|------|
| `joints` | joint_1 ~ joint_6 | 控制的6个关节 |
| `command_interfaces` | position | 位置命令接口 |
| `state_interfaces` | position, velocity | 状态反馈接口 |
| `allow_partial_joints_goal` | false | 不允许部分关节目标 |
| `open_loop_control` | true | 开环控制模式 |
| `stopped_velocity_tolerance` | 0.02 | 停止速度容差 |
| `goal_time` | 1.0 s | 目标到达时间容差 |
| 各关节 `trajectory/goal` 容差 | 0.15 / 0.05 rad | 轨迹/目标位置容差 |

#### empty.sdf 世界模型

- SDF 版本 1.10
- 物理引擎步长：0.001 s，实时因子 1.0
- 包含地面平面 (8m x 8m) 和方向光照明
- 加载 Gazebo 系统插件：Physics, UserCommands, SceneBroadcaster, Sensors

#### 依赖

| 依赖类型 | 包名 |
|----------|------|
| exec_depend | `controller_manager`, `gz_ros2_control`, `joint_state_broadcaster`, `joint_trajectory_controller`, `robot_arm_description`, `robot_arm_kinematics`, `robot_state_publisher`, `ros_gz_sim`, `rviz2`, `xacro` |

---

## 5. 包间依赖关系

```
                    ┌─────────────────────────┐
                    │  robot_arm_description   │
                    │  (URDF/Xacro 模型定义)    │
                    └────────────┬────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │ 被引用            │ 被引用            │
              ▼                  │                  ▼
┌──────────────────────────┐    │    ┌──────────────────────────┐
│  robot_arm_kinematics    │    │    │  robot_arm_bringup       │
│  (运动学算法与节点)       │◄───┘    │  (仿真启动与控制器配置)   │
└──────────────────────────┘         └──────────────────────────┘
              ▲                                 │
              │ exec_depend                     │ exec_depend
              └─────────────────────────────────┘
```

**依赖说明：**
- `robot_arm_bringup` 依赖 `robot_arm_description`（加载 URDF）和 `robot_arm_kinematics`（启动运动学节点）
- `robot_arm_kinematics` 不依赖其他自定义包，仅依赖 ROS 2 标准包和 Eigen
- `robot_arm_description` 不依赖其他自定义包，仅依赖 ROS 2 标准工具包

---

## 6. 话题与接口汇总

### 话题

| 话题名 | 消息类型 | 发布者 | 订阅者 | 方向 |
|--------|----------|--------|--------|------|
| `/target_pose` | `geometry_msgs/msg/PoseStamped` | `move_to_pose_demo` | `kinematics_node` | 目标位姿 |
| `/fk_pose` | `geometry_msgs/msg/PoseStamped` | `kinematics_node` | — | FK 验证位姿 |
| `/ik_status` | `std_msgs/msg/String` | `kinematics_node` | — | IK 求解状态 |
| `/joint_states` | `sensor_msgs/msg/JointState` | `joint_state_broadcaster` | `kinematics_node` | 关节状态 |

### Action

| Action 名 | 类型 | 客户端 | 服务器 |
|-----------|------|--------|--------|
| `/joint_trajectory_controller/follow_joint_trajectory` | `control_msgs/action/FollowJointTrajectory` | `kinematics_node` | `joint_trajectory_controller` |

---

## 7. 项目运行方式

### 7.1 环境要求

- **操作系统：** Ubuntu 24.04
- **ROS 发行版：** ROS 2 Jazzy
- **仿真器：** Gazebo (modern, via `ros_gz`)
- **依赖中间件：** `ros2_control`, `gz_ros2_control`

### 7.2 构建

```bash
cd robot
colcon build --symlink-install
source install/setup.bash
```

### 7.3 运行仿真

```bash
# 终端 1：启动 Gazebo 仿真 + 控制器 + 运动学节点
ros2 launch robot_arm_bringup sim.launch.py

# 终端 2：启动目标位姿演示
source install/setup.bash
ros2 run robot_arm_kinematics move_to_pose_demo
```

### 7.4 仅可视化模型（不启动仿真）

```bash
ros2 launch robot_arm_description display.launch.py
```

此模式启动 `robot_state_publisher`、`joint_state_publisher_gui`（可手动拖动关节）和 RViz。

### 7.5 运行 FK/IK 控制台测试

```bash
source install/setup.bash
ros2 run robot_arm_kinematics fk_ik_test
```

### 7.6 验收标准

- FK/IK 控制台测试通过（退出码 0）
- 位置误差 < 5 mm
- 姿态误差 < 3°
- Gazebo 中机械臂能跟随所有演示目标运动，无控制器报错

---

## 8. 关键算法参数速查

| 参数 | 值 | 所在文件 |
|------|-----|----------|
| DH: d1 | 0.0985 m | `kinematics.cpp:12`, `six_axis_arm.urdf.xacro:5` |
| DH: a2 | -0.408 m | `kinematics.cpp:13`, `six_axis_arm.urdf.xacro:6` |
| DH: a3 | -0.376 m | `kinematics.cpp:14`, `six_axis_arm.urdf.xacro:7` |
| DH: d4 | 0.1215 m | `kinematics.cpp:15`, `six_axis_arm.urdf.xacro:10` |
| DH: d5 | 0.1025 m | `kinematics.cpp:16`, `six_axis_arm.urdf.xacro:11` |
| DH: d6 | 0.094 m | `kinematics.cpp:17`, `six_axis_arm.urdf.xacro:12` |
| IK 最大迭代 | 250 | `kinematics.cpp:19` |
| IK 位置容差 | 5 mm | `kinematics.cpp:21` |
| IK 姿态容差 | 3° | `kinematics.cpp:22` |
| IK 阻尼系数 | 0.08 | `kinematics.cpp:23` |
| IK 单步上限 | 0.18 rad | `kinematics.cpp:24` |
| 轨迹插值点数 | 50 | `kinematics_node.cpp:164` |
| 轨迹总时长 | 5.0 s | `kinematics_node.cpp:165` |
| 控制器更新率 | 100 Hz | `controllers.yaml:3` |
| 目标发布间隔 | 8 s | `move_to_pose_demo.cpp:51` |
