#include <cmath>
#include <iostream>

#include "rclcpp/rclcpp.hpp"

#include "robot_arm_kinematics/kinematics.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  robot_arm_kinematics::SixAxisArmKinematics kinematics;

  robot_arm_kinematics::Vector6d requested_fk;
  requested_fk << 0.0, M_PI / 2.0, 0.0, M_PI / 2.0, 0.0, 0.0;
  const Eigen::Matrix4d requested_fk_pose = kinematics.forward(requested_fk);
  const Eigen::Matrix4d requested_fk_pose_dq =
    kinematics.forwardDualQuaternion(requested_fk).toTransform();
  const double dq_position_error =
    (requested_fk_pose.block<3, 1>(0, 3) - requested_fk_pose_dq.block<3, 1>(0, 3)).norm();
  const double dq_orientation_error =
    robot_arm_kinematics::SixAxisArmKinematics::orientationErrorNorm(
    requested_fk_pose,
    requested_fk_pose_dq);

  std::cout << "FK for [0, pi/2, 0, pi/2, 0, 0]:" << std::endl;
  std::cout << requested_fk_pose << std::endl << std::endl;
  std::cout << "Dual quaternion FK consistency:" << std::endl;
  std::cout << "Position difference (m): " << dq_position_error << std::endl;
  std::cout << "Orientation difference (rad): " << dq_orientation_error << std::endl
            << std::endl;

  robot_arm_kinematics::Vector6d test_q;
  test_q << 0.2, 0.35, -0.55, 0.75, -0.35, 0.25;
  const Eigen::Matrix4d target = kinematics.forward(test_q);
  const auto ik = kinematics.inverse(target, robot_arm_kinematics::Vector6d::Zero());
  const Eigen::Matrix4d recovered = kinematics.forward(ik.joints);
  const double position_error =
    (target.block<3, 1>(0, 3) - recovered.block<3, 1>(0, 3)).norm();
  const double orientation_error =
    robot_arm_kinematics::SixAxisArmKinematics::orientationErrorNorm(target, recovered);

  std::cout << "IK test target joints: " << test_q.transpose() << std::endl;
  std::cout << "IK converged: " << (ik.converged ? "true" : "false") << std::endl;
  std::cout << "IK result joints: " << ik.joints.transpose() << std::endl;
  std::cout << "Position error (m): " << position_error << std::endl;
  std::cout << "Orientation error (rad): " << orientation_error << std::endl;

  rclcpp::shutdown();

  if (dq_position_error > 1.0e-9 || dq_orientation_error > 1.0e-9 ||
    !ik.converged || position_error > 0.005 || orientation_error > 3.0 * M_PI / 180.0)
  {
    return 1;
  }
  return 0;
}
