#include <chrono>
#include <memory>
#include <vector>

#include <Eigen/Dense>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"

#include "robot_arm_kinematics/kinematics.hpp"

namespace
{
using robot_arm_kinematics::SixAxisArmKinematics;
using robot_arm_kinematics::Vector6d;

geometry_msgs::msg::Pose matrixToPoseMsg(const Eigen::Matrix4d & transform)
{
  geometry_msgs::msg::Pose pose;
  pose.position.x = transform(0, 3);
  pose.position.y = transform(1, 3);
  pose.position.z = transform(2, 3);

  Eigen::Quaterniond q(transform.block<3, 3>(0, 0));
  q.normalize();
  pose.orientation.x = q.x();
  pose.orientation.y = q.y();
  pose.orientation.z = q.z();
  pose.orientation.w = q.w();
  return pose;
}
}  // namespace

class MoveToPoseDemo : public rclcpp::Node
{
public:
  MoveToPoseDemo()
  : Node("move_to_pose_demo")
  {
    publisher_ = create_publisher<geometry_msgs::msg::PoseStamped>("/target_pose", 10);

    Vector6d q1;
    q1 << 0.25, -0.7, 1.25, -0.45, 0.3, 0.2;
    Vector6d q2;
    q2 << -0.35, -1.0, 1.55, -0.5, -0.35, -0.2;
    Vector6d q3;
    q3 << 0.5, -0.75, 1.3, -0.5, 0.25, 0.35;
    targets_ = {q1, q2, q3};

    timer_ = create_wall_timer(
      std::chrono::seconds(8),
      std::bind(&MoveToPoseDemo::publishNextTarget, this));

    publishNextTarget();
  }

private:
  void publishNextTarget()
  {
    const auto & q = targets_[target_index_ % targets_.size()];
    const Eigen::Matrix4d target = kinematics_.forward(q);

    geometry_msgs::msg::PoseStamped msg;
    msg.header.stamp = now();
    msg.header.frame_id = "base_link";
    msg.pose = matrixToPoseMsg(target);
    publisher_->publish(msg);

    RCLCPP_INFO(
      get_logger(),
      "Published target pose %zu at x=%.3f y=%.3f z=%.3f",
      target_index_ + 1,
      msg.pose.position.x,
      msg.pose.position.y,
      msg.pose.position.z);

    ++target_index_;
  }

  SixAxisArmKinematics kinematics_;
  std::vector<Vector6d> targets_;
  std::size_t target_index_{0};
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MoveToPoseDemo>());
  rclcpp::shutdown();
  return 0;
}
