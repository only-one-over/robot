#include "robot_arm_kinematics/kinematics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace robot_arm_kinematics
{
namespace
{
constexpr double kD1 = 0.0985;
constexpr double kA2 = -0.408;
constexpr double kA3 = -0.376;
constexpr double kD4 = 0.1215;
constexpr double kD5 = 0.1025;
constexpr double kD6 = 0.094;
constexpr double kPi = 3.14159265358979323846;
constexpr int kMaxIterations = 250;
constexpr double kFiniteDifferenceStep = 1.0e-5;
constexpr double kPositionTolerance = 0.005;
constexpr double kOrientationTolerance = 3.0 * kPi / 180.0;
constexpr double kDamping = 0.08;
constexpr double kMaxStep = 0.18;

double clamp(double value, double low, double high)
{
  return std::max(low, std::min(value, high));
}

Eigen::Quaterniond addQuaternions(
  const Eigen::Quaterniond & lhs,
  const Eigen::Quaterniond & rhs)
{
  return Eigen::Quaterniond(
    lhs.w() + rhs.w(),
    lhs.x() + rhs.x(),
    lhs.y() + rhs.y(),
    lhs.z() + rhs.z());
}

Eigen::Quaterniond scaleQuaternion(const Eigen::Quaterniond & q, double scale)
{
  return Eigen::Quaterniond(q.w() * scale, q.x() * scale, q.y() * scale, q.z() * scale);
}
}  // namespace

DualQuaternion DualQuaternion::identity()
{
  return DualQuaternion{};
}

DualQuaternion DualQuaternion::fromTransform(const Eigen::Matrix4d & transform)
{
  Eigen::Quaterniond rotation(transform.block<3, 3>(0, 0));
  rotation.normalize();
  return fromRotationTranslation(rotation, transform.block<3, 1>(0, 3));
}

DualQuaternion DualQuaternion::fromRotationTranslation(
  const Eigen::Quaterniond & rotation,
  const Eigen::Vector3d & translation)
{
  DualQuaternion result;
  result.real = rotation.normalized();
  const Eigen::Quaterniond translation_quat(
    0.0,
    translation.x(),
    translation.y(),
    translation.z());
  result.dual = scaleQuaternion(translation_quat * result.real, 0.5);
  return result;
}

DualQuaternion DualQuaternion::normalized() const
{
  DualQuaternion result = *this;
  const double norm = result.real.norm();
  if (norm > 0.0) {
    result.real.coeffs() /= norm;
    result.dual.coeffs() /= norm;
  }
  return result;
}

DualQuaternion DualQuaternion::operator*(const DualQuaternion & other) const
{
  DualQuaternion result;
  result.real = real * other.real;
  result.dual = addQuaternions(real * other.dual, dual * other.real);
  return result.normalized();
}

Eigen::Vector3d DualQuaternion::translation() const
{
  const DualQuaternion normalized_dq = normalized();
  const Eigen::Quaterniond translation_quat =
    scaleQuaternion(normalized_dq.dual * normalized_dq.real.conjugate(), 2.0);
  return Eigen::Vector3d(translation_quat.x(), translation_quat.y(), translation_quat.z());
}

Eigen::Matrix4d DualQuaternion::toTransform() const
{
  const DualQuaternion normalized_dq = normalized();
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  transform.block<3, 3>(0, 0) = normalized_dq.real.toRotationMatrix();
  transform.block<3, 1>(0, 3) = normalized_dq.translation();
  return transform;
}

SixAxisArmKinematics::SixAxisArmKinematics()
: joint_names_({"joint_1", "joint_2", "joint_3", "joint_4", "joint_5", "joint_6"})
{
  lower_limits_ << -kPi, -2.61799, -2.61799, -kPi, -kPi, -kPi;
  upper_limits_ << kPi, 2.61799, 2.61799, kPi, kPi, kPi;
}

Eigen::Matrix4d SixAxisArmKinematics::translate(double x, double y, double z) const
{
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  transform(0, 3) = x;
  transform(1, 3) = y;
  transform(2, 3) = z;
  return transform;
}

Eigen::Matrix4d SixAxisArmKinematics::rotate(const Eigen::Vector3d & axis, double angle) const
{
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  transform.block<3, 3>(0, 0) =
    Eigen::AngleAxisd(angle, axis.normalized()).toRotationMatrix();
  return transform;
}

Eigen::Matrix4d SixAxisArmKinematics::forward(const Vector6d & q) const
{
  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();

  transform *= translate(0.0, 0.0, kD1) * rotate(Eigen::Vector3d::UnitZ(), q(0));
  transform *= translate(0.0, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(1));
  transform *= translate(kA2, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(2));
  transform *= translate(kA3, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(3));
  transform *= translate(0.0, -kD4, 0.0) * rotate(Eigen::Vector3d(0.0, 0.0, -1.0), q(4));
  transform *= translate(0.0, kD4, -kD5) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(5));
  transform *= translate(0.0, -(kD4 + kD6), 0.0);

  return transform;
}

DualQuaternion SixAxisArmKinematics::forwardDualQuaternion(const Vector6d & q) const
{
  DualQuaternion transform = DualQuaternion::identity();

  transform = transform * DualQuaternion::fromTransform(
    translate(0.0, 0.0, kD1) * rotate(Eigen::Vector3d::UnitZ(), q(0)));
  transform = transform * DualQuaternion::fromTransform(
    translate(0.0, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(1)));
  transform = transform * DualQuaternion::fromTransform(
    translate(kA2, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(2)));
  transform = transform * DualQuaternion::fromTransform(
    translate(kA3, 0.0, 0.0) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(3)));
  transform = transform * DualQuaternion::fromTransform(
    translate(0.0, -kD4, 0.0) * rotate(Eigen::Vector3d(0.0, 0.0, -1.0), q(4)));
  transform = transform * DualQuaternion::fromTransform(
    translate(0.0, kD4, -kD5) * rotate(Eigen::Vector3d(0.0, -1.0, 0.0), q(5)));
  transform = transform * DualQuaternion::fromTransform(translate(0.0, -(kD4 + kD6), 0.0));

  return transform.normalized();
}

Eigen::Matrix<double, 6, 1> SixAxisArmKinematics::poseError(
  const Eigen::Matrix4d & target,
  const Eigen::Matrix4d & actual)
{
  Eigen::Matrix<double, 6, 1> error;
  error.head<3>() = target.block<3, 1>(0, 3) - actual.block<3, 1>(0, 3);

  const Eigen::Matrix3d target_rot = target.block<3, 3>(0, 0);
  const Eigen::Matrix3d actual_rot = actual.block<3, 3>(0, 0);
  const Eigen::Vector3d orientation_error =
    0.5 *
    (actual_rot.col(0).cross(target_rot.col(0)) +
     actual_rot.col(1).cross(target_rot.col(1)) +
     actual_rot.col(2).cross(target_rot.col(2)));
  error.tail<3>() = orientation_error;
  return error;
}

double SixAxisArmKinematics::orientationErrorNorm(
  const Eigen::Matrix4d & target,
  const Eigen::Matrix4d & actual)
{
  const Eigen::Matrix3d relative =
    actual.block<3, 3>(0, 0).transpose() * target.block<3, 3>(0, 0);
  const double trace_value = clamp((relative.trace() - 1.0) * 0.5, -1.0, 1.0);
  return std::acos(trace_value);
}

Vector6d SixAxisArmKinematics::clampToLimits(const Vector6d & q) const
{
  Vector6d clamped = q;
  for (int i = 0; i < 6; ++i) {
    clamped(i) = clamp(clamped(i), lower_limits_(i), upper_limits_(i));
  }
  return clamped;
}

IkResult SixAxisArmKinematics::inverse(const Eigen::Matrix4d & target, const Vector6d & seed) const
{
  IkResult best;
  best.joints = clampToLimits(seed);
  best.position_error = std::numeric_limits<double>::infinity();
  best.orientation_error = std::numeric_limits<double>::infinity();

  std::vector<Vector6d> seeds;
  seeds.push_back(clampToLimits(seed));
  seeds.push_back(Vector6d::Zero());
  Vector6d elbow_up = Vector6d::Zero();
  elbow_up << 0.0, 0.6, -0.8, 0.4, 0.0, 0.0;
  seeds.push_back(elbow_up);
  Vector6d elbow_down = Vector6d::Zero();
  elbow_down << 0.0, -0.6, 0.8, -0.4, 0.0, 0.0;
  seeds.push_back(elbow_down);

  for (const auto & initial_seed : seeds) {
    Vector6d q = clampToLimits(initial_seed);
    IkResult candidate;
    candidate.joints = q;

    for (int iteration = 0; iteration < kMaxIterations; ++iteration) {
      const Eigen::Matrix4d actual = forward(q);
      const Eigen::Matrix<double, 6, 1> error = poseError(target, actual);
      const double position_error = error.head<3>().norm();
      const double orientation_error = orientationErrorNorm(target, actual);

      candidate.iterations = iteration;
      candidate.position_error = position_error;
      candidate.orientation_error = orientation_error;
      candidate.joints = q;

      if (position_error < kPositionTolerance && orientation_error < kOrientationTolerance) {
        candidate.converged = true;
        candidate.message = "IK converged";
        break;
      }

      Matrix6d jacobian;
      for (int column = 0; column < 6; ++column) {
        Vector6d q_plus = q;
        q_plus(column) += kFiniteDifferenceStep;
        q_plus = clampToLimits(q_plus);
        const Eigen::Matrix<double, 6, 1> error_plus = poseError(target, forward(q_plus));
        jacobian.col(column) = (error_plus - error) / kFiniteDifferenceStep;
      }

      const Matrix6d normal =
        jacobian * jacobian.transpose() + kDamping * kDamping * Matrix6d::Identity();
      Vector6d delta = -jacobian.transpose() * normal.ldlt().solve(error);

      for (int i = 0; i < 6; ++i) {
        delta(i) = clamp(delta(i), -kMaxStep, kMaxStep);
      }

      q = clampToLimits(q + delta);
    }

    if (!candidate.converged) {
      const Eigen::Matrix4d actual = forward(q);
      candidate.joints = q;
      candidate.position_error = (target.block<3, 1>(0, 3) - actual.block<3, 1>(0, 3)).norm();
      candidate.orientation_error = orientationErrorNorm(target, actual);
      candidate.message = "IK did not converge";
    }

    const bool better =
      (candidate.converged && !best.converged) ||
      ((candidate.converged == best.converged) &&
      (candidate.position_error + candidate.orientation_error <
      best.position_error + best.orientation_error));
    if (better) {
      best = candidate;
    }
  }

  if (!best.converged && best.message.empty()) {
    std::ostringstream stream;
    stream << "IK failed: position error " << best.position_error
           << " m, orientation error " << best.orientation_error << " rad";
    best.message = stream.str();
  }
  return best;
}

std::vector<Vector6d> SixAxisArmKinematics::quinticTrajectory(
  const Vector6d & start,
  const Vector6d & goal,
  int point_count) const
{
  const int count = std::max(2, point_count);
  std::vector<Vector6d> points;
  points.reserve(static_cast<std::size_t>(count));

  for (int i = 0; i < count; ++i) {
    const double s = static_cast<double>(i) / static_cast<double>(count - 1);
    const double blend = 10.0 * std::pow(s, 3) - 15.0 * std::pow(s, 4) + 6.0 * std::pow(s, 5);
    points.push_back(start + blend * (goal - start));
  }

  return points;
}

const std::array<std::string, 6> & SixAxisArmKinematics::jointNames() const
{
  return joint_names_;
}

const Vector6d & SixAxisArmKinematics::lowerLimits() const
{
  return lower_limits_;
}

const Vector6d & SixAxisArmKinematics::upperLimits() const
{
  return upper_limits_;
}

}  // namespace robot_arm_kinematics
