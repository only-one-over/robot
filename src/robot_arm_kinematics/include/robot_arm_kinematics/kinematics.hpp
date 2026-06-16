#ifndef ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_
#define ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_

#include <array>
#include <string>
#include <vector>

#include <Eigen/Dense>

namespace robot_arm_kinematics
{

using Vector6d = Eigen::Matrix<double, 6, 1>;
using Matrix6d = Eigen::Matrix<double, 6, 6>;

struct IkResult
{
  bool converged{false};
  Vector6d joints{Vector6d::Zero()};
  int iterations{0};
  double position_error{0.0};
  double orientation_error{0.0};
  std::string message;
};

class SixAxisArmKinematics
{
public:
  SixAxisArmKinematics();

  Eigen::Matrix4d forward(const Vector6d & q) const;
  IkResult inverse(const Eigen::Matrix4d & target, const Vector6d & seed) const;
  std::vector<Vector6d> quinticTrajectory(
    const Vector6d & start,
    const Vector6d & goal,
    int point_count) const;

  const std::array<std::string, 6> & jointNames() const;
  const Vector6d & lowerLimits() const;
  const Vector6d & upperLimits() const;

  static Eigen::Matrix<double, 6, 1> poseError(
    const Eigen::Matrix4d & target,
    const Eigen::Matrix4d & actual);
  static double orientationErrorNorm(
    const Eigen::Matrix4d & target,
    const Eigen::Matrix4d & actual);

private:
  Eigen::Matrix4d translate(double x, double y, double z) const;
  Eigen::Matrix4d rotate(const Eigen::Vector3d & axis, double angle) const;
  Vector6d clampToLimits(const Vector6d & q) const;

  std::array<std::string, 6> joint_names_;
  Vector6d lower_limits_;
  Vector6d upper_limits_;
};

}  // namespace robot_arm_kinematics

#endif  // ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_

