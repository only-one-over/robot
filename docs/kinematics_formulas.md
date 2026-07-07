---
title: "六自由度机械臂运动学公式整理"
lang: zh-CN
---

# 六自由度机械臂运动学公式整理

本文档整理课程论文中可直接使用的机器人运动学公式。公式采用 LaTeX 书写，编号形式为 `(1)(2)(3)`，可通过 Pandoc 转换为 Word 文档。本文中的机械臂参数与项目实现保持一致：

| 参数 | 数值 / m | 含义 |
| --- | ---: | --- |
| $d_1$ | 0.0985 | 基座到第一关节的 $z$ 向偏移 |
| $a_2$ | -0.408 | 第二、三关节之间的 $x$ 向连杆长度 |
| $a_3$ | -0.376 | 第三、四关节之间的 $x$ 向连杆长度 |
| $d_4$ | 0.1215 | 腕部局部偏移 |
| $d_5$ | 0.1025 | 第五关节附近的工具侧偏移 |
| $d_6$ | 0.094 | 第六关节到末端工具坐标系的偏移 |

## 1. 位姿与齐次变换

空间刚体位姿可用齐次变换矩阵表示：

$$
{}^{0}\mathbf{T}_{e}
=
\begin{bmatrix}
{}^{0}\mathbf{R}_{e} & {}^{0}\mathbf{p}_{e} \\
\mathbf{0}_{1\times 3} & 1
\end{bmatrix}
\in SE(3)
\tag{1}
$$

其中，${}^{0}\mathbf{T}_{e}$ 表示末端坐标系 $e$ 相对于基坐标系 $0$ 的位姿；${}^{0}\mathbf{R}_{e}\in SO(3)$ 为 $3\times3$ 旋转矩阵，描述末端姿态；${}^{0}\mathbf{p}_{e}\in\mathbb{R}^{3}$ 为位置向量；$SE(3)$ 表示三维刚体运动群。

关节变量向量定义为：

$$
\mathbf{q}
=
\begin{bmatrix}
q_1 & q_2 & q_3 & q_4 & q_5 & q_6
\end{bmatrix}^{\mathrm{T}}
\in \mathbb{R}^{6}
\tag{2}
$$

其中，$\mathbf{q}$ 为六自由度机械臂的关节角向量；$q_i$ 表示第 $i$ 个转动关节的角度，单位为弧度；上标 $\mathrm{T}$ 表示矩阵或向量转置。

平移变换矩阵写为：

$$
\operatorname{Trans}(x,y,z)
=
\begin{bmatrix}
1 & 0 & 0 & x \\
0 & 1 & 0 & y \\
0 & 0 & 1 & z \\
0 & 0 & 0 & 1
\end{bmatrix}
\tag{3}
$$

其中，$\operatorname{Trans}(x,y,z)$ 表示沿局部坐标系 $x$、$y$、$z$ 方向分别平移 $x$、$y$、$z$ 的齐次变换；前三行前三列为单位矩阵，表示该变换不改变姿态。

绕单位轴 $\mathbf{u}$ 旋转角度 $\theta$ 的齐次变换可写为：

$$
\operatorname{Rot}(\mathbf{u},\theta)
=
\begin{bmatrix}
\mathbf{R}(\mathbf{u},\theta) & \mathbf{0}_{3\times 1} \\
\mathbf{0}_{1\times 3} & 1
\end{bmatrix}
\tag{4}
$$

其中，$\operatorname{Rot}(\mathbf{u},\theta)$ 为绕单位方向向量 $\mathbf{u}$ 的旋转变换；$\theta$ 为旋转角；$\mathbf{R}(\mathbf{u},\theta)$ 为对应的 $3\times3$ 旋转矩阵；$\mathbf{0}$ 表示零向量或零矩阵。

## 2. DH 与等效 MDH 变换

标准 DH 单节变换常写为：

$$
{}^{i-1}\mathbf{A}_{i}^{\mathrm{DH}}
=
\operatorname{Rot}_{z}(\theta_i)
\operatorname{Trans}_{z}(d_i)
\operatorname{Trans}_{x}(a_i)
\operatorname{Rot}_{x}(\alpha_i)
\tag{5}
$$

其中，${}^{i-1}\mathbf{A}_{i}^{\mathrm{DH}}$ 表示标准 DH 方法中从坐标系 $i-1$ 到坐标系 $i$ 的变换；$\theta_i$ 为关节转角；$d_i$ 为沿 $z_i$ 轴的偏移；$a_i$ 为沿 $x_i$ 轴的连杆长度；$\alpha_i$ 为相邻 $z$ 轴之间的扭转角。

改进 DH，也称 MDH，常用的单节变换可写为：

$$
{}^{i-1}\mathbf{A}_{i}^{\mathrm{MDH}}
=
\operatorname{Trans}_{x}(a_{i-1})
\operatorname{Rot}_{x}(\alpha_{i-1})
\operatorname{Trans}_{z}(d_i)
\operatorname{Rot}_{z}(\theta_i)
\tag{6}
$$

其中，${}^{i-1}\mathbf{A}_{i}^{\mathrm{MDH}}$ 表示改进 DH 方法中的相邻坐标系变换；$a_{i-1}$ 和 $\alpha_{i-1}$ 描述上一连杆的几何关系；$d_i$ 和 $\theta_i$ 描述当前关节的位移或转角。本项目采用与 MDH 思想一致的逐关节局部变换链，使数学模型更容易与 URDF 中的 `joint origin` 和 `joint axis` 对齐。

## 3. 本项目的正运动学变换链

本项目中第 1 关节到第 6 关节及工具坐标系的局部变换定义为：

$$
\begin{aligned}
\mathbf{T}_1 &= \operatorname{Trans}(0,0,d_1)\operatorname{Rot}(\mathbf{e}_z,q_1),\\
\mathbf{T}_2 &= \operatorname{Rot}(-\mathbf{e}_y,q_2),\\
\mathbf{T}_3 &= \operatorname{Trans}(a_2,0,0)\operatorname{Rot}(-\mathbf{e}_y,q_3),\\
\mathbf{T}_4 &= \operatorname{Trans}(a_3,0,0)\operatorname{Rot}(-\mathbf{e}_y,q_4),\\
\mathbf{T}_5 &= \operatorname{Trans}(0,-d_4,0)\operatorname{Rot}(-\mathbf{e}_z,q_5),\\
\mathbf{T}_6 &= \operatorname{Trans}(0,d_4,-d_5)\operatorname{Rot}(-\mathbf{e}_y,q_6),\\
\mathbf{T}_{\mathrm{tool}} &= \operatorname{Trans}(0,-(d_4+d_6),0).
\end{aligned}
\tag{7}
$$

其中，$\mathbf{T}_i$ 表示第 $i$ 个局部关节变换；$\mathbf{T}_{\mathrm{tool}}$ 表示第六关节到末端工具坐标系 `tool0` 的固定变换；$\mathbf{e}_x$、$\mathbf{e}_y$、$\mathbf{e}_z$ 分别为局部坐标系 $x$、$y$、$z$ 轴的单位向量；$d_1,a_2,a_3,d_4,d_5,d_6$ 为机械臂几何参数。

因此，末端相对于基座的正运动学结果为：

$$
{}^{0}\mathbf{T}_{e}(\mathbf{q})
=
\mathbf{T}_1
\mathbf{T}_2
\mathbf{T}_3
\mathbf{T}_4
\mathbf{T}_5
\mathbf{T}_6
\mathbf{T}_{\mathrm{tool}}
\tag{8}
$$

其中，${}^{0}\mathbf{T}_{e}(\mathbf{q})$ 为给定关节角 $\mathbf{q}$ 时的末端齐次变换矩阵；连乘顺序从基座开始，依次经过六个关节，最后到达末端工具坐标系。该式对应代码中的 `SixAxisArmKinematics::forward()`。

从齐次矩阵中提取末端位置与姿态的方法为：

$$
{}^{0}\mathbf{p}_{e}
=
{}^{0}\mathbf{T}_{e}(1:3,4),
\qquad
{}^{0}\mathbf{R}_{e}
=
{}^{0}\mathbf{T}_{e}(1:3,1:3)
\tag{9}
$$

其中，${}^{0}\mathbf{p}_{e}$ 为末端位置；${}^{0}\mathbf{R}_{e}$ 为末端姿态；记号 $(1:3,4)$ 表示取齐次矩阵前三行第四列，$(1:3,1:3)$ 表示取前三行前三列。

## 4. 对偶四元数位姿表达

刚体位姿也可用单位对偶四元数表示：

$$
\hat{\mathbf{q}}
=
\mathbf{q}_{r}
+
\varepsilon\mathbf{q}_{d},
\qquad
\varepsilon^2=0
\tag{10}
$$

其中，$\hat{\mathbf{q}}$ 为对偶四元数；$\mathbf{q}_{r}$ 为实部四元数，表示旋转；$\mathbf{q}_{d}$ 为对偶部四元数，编码平移；$\varepsilon$ 为对偶单位，满足 $\varepsilon^2=0$。

由旋转四元数与平移向量构造对偶四元数的公式为：

$$
\mathbf{q}_{d}
=
\frac{1}{2}
\mathbf{p}^{\ast}
\otimes
\mathbf{q}_{r},
\qquad
\mathbf{p}^{\ast}
=
0+p_x\mathbf{i}+p_y\mathbf{j}+p_z\mathbf{k}
\tag{11}
$$

其中，$\mathbf{q}_{r}$ 为单位旋转四元数；$\mathbf{q}_{d}$ 为对偶部；$\mathbf{p}^{\ast}$ 为由平移向量 $\mathbf{p}=[p_x,p_y,p_z]^{\mathrm{T}}$ 构成的纯四元数；$\otimes$ 表示四元数乘法；$\mathbf{i}$、$\mathbf{j}$、$\mathbf{k}$ 为四元数虚部基。

由对偶四元数恢复平移向量的公式为：

$$
\mathbf{p}^{\ast}
=
2
\mathbf{q}_{d}
\otimes
\mathbf{q}_{r}^{\ast}
\tag{12}
$$

其中，$\mathbf{q}_{r}^{\ast}$ 表示旋转四元数 $\mathbf{q}_{r}$ 的共轭；$\mathbf{p}^{\ast}$ 的虚部即为平移向量 $\mathbf{p}$。该公式对应代码中的 `DualQuaternion::translation()`。

位姿连乘在对偶四元数中可写为：

$$
\hat{\mathbf{q}}_{0e}
=
\hat{\mathbf{q}}_1
\otimes
\hat{\mathbf{q}}_2
\otimes
\cdots
\otimes
\hat{\mathbf{q}}_6
\otimes
\hat{\mathbf{q}}_{\mathrm{tool}}
\tag{13}
$$

其中，$\hat{\mathbf{q}}_{0e}$ 表示从基座到末端的整体位姿；$\hat{\mathbf{q}}_i$ 为第 $i$ 个关节局部变换对应的对偶四元数；$\hat{\mathbf{q}}_{\mathrm{tool}}$ 为工具端固定变换对应的对偶四元数。该式与式 (8) 在数学上等价，对应代码中的 `SixAxisArmKinematics::forwardDualQuaternion()`。

## 5. 逆运动学目标与误差定义

逆运动学的目标是寻找一组关节角，使正运动学结果逼近期望末端位姿：

$$
\mathbf{q}^{\ast}
=
\arg\min_{\mathbf{q}}
\left\|
\mathbf{e}(\mathbf{q})
\right\|_2
\tag{14}
$$

其中，$\mathbf{q}^{\ast}$ 为逆运动学求得的关节角；$\arg\min$ 表示使目标函数最小的变量取值；$\mathbf{e}(\mathbf{q})$ 为当前位姿与目标位姿之间的六维误差向量；$\|\cdot\|_2$ 表示欧氏范数。

位置误差定义为：

$$
\mathbf{e}_{p}
=
{}^{0}\mathbf{p}_{d}
-
{}^{0}\mathbf{p}_{e}(\mathbf{q})
\tag{15}
$$

其中，$\mathbf{e}_{p}$ 为三维位置误差；${}^{0}\mathbf{p}_{d}$ 为目标末端位置；${}^{0}\mathbf{p}_{e}(\mathbf{q})$ 为由当前关节角 $\mathbf{q}$ 通过正运动学计算得到的末端位置。

姿态误差定义为：

$$
\mathbf{e}_{R}
=
\frac{1}{2}
\left(
\mathbf{r}_{e,1}\times\mathbf{r}_{d,1}
+
\mathbf{r}_{e,2}\times\mathbf{r}_{d,2}
+
\mathbf{r}_{e,3}\times\mathbf{r}_{d,3}
\right)
\tag{16}
$$

其中，$\mathbf{e}_{R}$ 为三维姿态误差向量；$\mathbf{r}_{e,j}$ 为当前末端旋转矩阵 ${}^{0}\mathbf{R}_{e}$ 的第 $j$ 列；$\mathbf{r}_{d,j}$ 为目标旋转矩阵 ${}^{0}\mathbf{R}_{d}$ 的第 $j$ 列；$\times$ 表示向量叉乘。该式对应代码中的 `poseError()` 姿态误差计算。

六维位姿误差向量为：

$$
\mathbf{e}
=
\begin{bmatrix}
\mathbf{e}_{p} \\
\mathbf{e}_{R}
\end{bmatrix}
\in\mathbb{R}^{6}
\tag{17}
$$

其中，$\mathbf{e}$ 为逆运动学迭代使用的总误差；前三维为位置误差，后三维为姿态误差；$\mathbb{R}^{6}$ 表示六维实向量空间。

姿态角误差也可由相对旋转矩阵计算：

$$
\theta_{R}
=
\cos^{-1}
\left(
\frac{\operatorname{tr}\left({}^{0}\mathbf{R}_{e}^{\mathrm{T}}{}^{0}\mathbf{R}_{d}\right)-1}{2}
\right)
\tag{18}
$$

其中，$\theta_R$ 为当前姿态到目标姿态之间的角度误差；$\operatorname{tr}(\cdot)$ 表示矩阵迹；${}^{0}\mathbf{R}_{e}^{\mathrm{T}}{}^{0}\mathbf{R}_{d}$ 为相对旋转矩阵。该式对应代码中的 `orientationErrorNorm()`，用于输出更直观的角度误差。

## 6. Jacobian 与阻尼最小二乘逆解

在当前关节角附近，末端误差与关节增量之间可作一阶线性近似：

$$
\mathbf{e}(\mathbf{q}+\Delta\mathbf{q})
\approx
\mathbf{e}(\mathbf{q})
+
\mathbf{J}(\mathbf{q})\Delta\mathbf{q}
\tag{19}
$$

其中，$\Delta\mathbf{q}$ 为关节角增量；$\mathbf{J}(\mathbf{q})\in\mathbb{R}^{6\times6}$ 为机械臂 Jacobian 矩阵；该矩阵描述关节角微小变化对末端位姿误差的影响。

有限差分 Jacobian 的第 $i$ 列计算为：

$$
\mathbf{J}_{:,i}
=
\frac{
\mathbf{e}(\mathbf{q}+h\mathbf{e}_i)
-
\mathbf{e}(\mathbf{q})
}{h}
\tag{20}
$$

其中，$\mathbf{J}_{:,i}$ 表示 Jacobian 的第 $i$ 列；$h$ 为有限差分步长，本项目取 $h=10^{-5}$；$\mathbf{e}_i$ 为第 $i$ 个标准基向量。该式对应代码中对每个关节增加微小扰动后重新计算误差的过程。

阻尼最小二乘法给出的关节角增量为：

$$
\Delta\mathbf{q}
=
-
\mathbf{J}^{\mathrm{T}}
\left(
\mathbf{J}\mathbf{J}^{\mathrm{T}}
+
\lambda^2\mathbf{I}
\right)^{-1}
\mathbf{e}
\tag{21}
$$

其中，$\Delta\mathbf{q}$ 为一次迭代中的关节更新量；$\mathbf{J}^{\mathrm{T}}$ 为 Jacobian 转置；$\lambda$ 为阻尼系数，本项目取 $\lambda=0.08$；$\mathbf{I}$ 为 $6\times6$ 单位矩阵；负号表示沿减小误差的方向更新。阻尼项 $\lambda^2\mathbf{I}$ 可减小奇异位形附近的数值放大。

关节角迭代更新可写为：

$$
\mathbf{q}_{k+1}
=
\operatorname{clip}
\left(
\mathbf{q}_{k}
+
\operatorname{sat}(\Delta\mathbf{q}),
\mathbf{q}_{\min},
\mathbf{q}_{\max}
\right)
\tag{22}
$$

其中，$\mathbf{q}_k$ 为第 $k$ 次迭代的关节角；$\mathbf{q}_{k+1}$ 为更新后的关节角；$\operatorname{sat}(\cdot)$ 表示单步最大增量限制，本项目最大单步关节变化为 $0.18\,\mathrm{rad}$；$\operatorname{clip}(\cdot)$ 表示关节限位裁剪；$\mathbf{q}_{\min}$ 和 $\mathbf{q}_{\max}$ 分别为关节下限和上限。

IK 收敛条件为：

$$
\left\|\mathbf{e}_{p}\right\|_2 < 0.005\,\mathrm{m},
\qquad
\theta_R < 3^{\circ}
\tag{23}
$$

其中，$\|\mathbf{e}_{p}\|_2$ 为位置误差范数；$\theta_R$ 为姿态角误差；$0.005\,\mathrm{m}$ 对应 $5\,\mathrm{mm}$ 的位置精度要求；$3^{\circ}$ 为姿态误差阈值。

## 7. 五次多项式关节轨迹

归一化时间定义为：

$$
\tau
=
\frac{t}{T},
\qquad
0\leq \tau \leq 1
\tag{24}
$$

其中，$t$ 为当前时间；$T$ 为轨迹总时长；$\tau$ 为归一化时间变量，用于把不同持续时间的轨迹统一映射到 $[0,1]$ 区间。

五次多项式插值因子定义为：

$$
s(\tau)
=
10\tau^3
-
15\tau^4
+
6\tau^5
\tag{25}
$$

其中，$s(\tau)$ 为从起点到终点的归一化插值比例；当 $\tau=0$ 时 $s=0$，当 $\tau=1$ 时 $s=1$。该函数可保证轨迹起点和终点处速度、加速度均为零。

关节空间轨迹为：

$$
\mathbf{q}(t)
=
\mathbf{q}_{s}
+
s(\tau)
\left(
\mathbf{q}_{g}
-
\mathbf{q}_{s}
\right)
\tag{26}
$$

其中，$\mathbf{q}(t)$ 为时刻 $t$ 的关节角；$\mathbf{q}_s$ 为起始关节角；$\mathbf{q}_g$ 为目标关节角；$s(\tau)$ 为式 (25) 定义的五次插值因子。该式对应代码中的 `quinticTrajectory()`。

五次插值的边界条件为：

$$
s(0)=0,\quad
s(1)=1,\quad
\dot{s}(0)=\dot{s}(1)=0,\quad
\ddot{s}(0)=\ddot{s}(1)=0
\tag{27}
$$

其中，$\dot{s}$ 表示对时间的一阶导数，对应归一化速度；$\ddot{s}$ 表示对时间的二阶导数，对应归一化加速度。该边界条件使机械臂能够平滑起动和平滑停止，适合发送给 `joint_trajectory_controller` 执行。

## 8. Markdown 转 Word 建议

可使用以下命令将本文档转换为 Word：

```bash
pandoc docs/kinematics_formulas.md -o docs/kinematics_formulas.docx --from markdown+tex_math_dollars
```

若需要将公式章节并入完整论文，可先将本文内容复制到论文 Markdown 正文中，再使用相同的 Pandoc 命令导出为 `.docx`。
