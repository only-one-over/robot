#ifndef ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_
#define ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_

#include <array>
#include <string>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace robot_arm_kinematics
{

using Vector6d = Eigen::Matrix<double, 6, 1>;
using Matrix6d = Eigen::Matrix<double, 6, 6>;

struct DualQuaternion
{
  Eigen::Quaterniond real{Eigen::Quaterniond::Identity()};
  Eigen::Quaterniond dual{0.0, 0.0, 0.0, 0.0};

  static DualQuaternion identity();
  static DualQuaternion fromTransform(const Eigen::Matrix4d & transform);
  static DualQuaternion fromRotationTranslation(
    const Eigen::Quaterniond & rotation,
    const Eigen::Vector3d & translation);

  DualQuaternion normalized() const;
  DualQuaternion operator*(const DualQuaternion & other) const;
  Eigen::Matrix4d toTransform() const;
  Eigen::Vector3d translation() const;
};

struct IkResult
{
  bool converged{false};
  Vector6d joints{Vector6d::Zero()};
  int iterations{0};
  double position_error{0.0};
  double orientation_error{0.0};
  std::string message;
};

enum class IkMethod
{
  kJacobianTranspose,
  kPseudoinverse,
  kDampedLeastSquares
};

class SixAxisArmKinematics
{
public:
  SixAxisArmKinematics();

  Eigen::Matrix4d forward(const Vector6d & q) const;
  DualQuaternion forwardDualQuaternion(const Vector6d & q) const;
  IkResult inverse(const Eigen::Matrix4d & target, const Vector6d & seed) const;
  IkResult inverseWithMethod(
    const Eigen::Matrix4d & target,
    const Vector6d & seed,
    IkMethod method,
    bool use_multi_start = false) const;
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
  static const char * ikMethodName(IkMethod method);

private:
  Eigen::Matrix4d translate(double x, double y, double z) const;
  Eigen::Matrix4d rotate(const Eigen::Vector3d & axis, double angle) const;
  Matrix6d numericalJacobian(
    const Eigen::Matrix4d & target,
    const Vector6d & q,
    const Vector6d & error) const;
  Vector6d calculateIkStep(
    const Matrix6d & jacobian,
    const Vector6d & error,
    IkMethod method) const;
  Vector6d clampToLimits(const Vector6d & q) const;

  std::array<std::string, 6> joint_names_;
  Vector6d lower_limits_;
  Vector6d upper_limits_;
};

}  // namespace robot_arm_kinematics

#endif  // ROBOT_ARM_KINEMATICS__KINEMATICS_HPP_
