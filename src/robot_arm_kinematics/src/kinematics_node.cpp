#include <chrono>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <Eigen/Dense>

#include "control_msgs/action/follow_joint_trajectory.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/string.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

#include "robot_arm_kinematics/kinematics.hpp"

namespace
{
using FollowJointTrajectory = control_msgs::action::FollowJointTrajectory;
using GoalHandleFollowJointTrajectory = rclcpp_action::ClientGoalHandle<FollowJointTrajectory>;
using robot_arm_kinematics::SixAxisArmKinematics;
using robot_arm_kinematics::Vector6d;

Eigen::Matrix4d poseMsgToMatrix(const geometry_msgs::msg::Pose & pose)
{
  Eigen::Quaterniond q(
    pose.orientation.w,
    pose.orientation.x,
    pose.orientation.y,
    pose.orientation.z);
  q.normalize();

  Eigen::Matrix4d transform = Eigen::Matrix4d::Identity();
  transform.block<3, 3>(0, 0) = q.toRotationMatrix();
  transform(0, 3) = pose.position.x;
  transform(1, 3) = pose.position.y;
  transform(2, 3) = pose.position.z;
  return transform;
}

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

std::string vectorToString(const Vector6d & q)
{
  std::ostringstream stream;
  stream << "[";
  for (int i = 0; i < 6; ++i) {
    stream << q(i);
    if (i != 5) {
      stream << ", ";
    }
  }
  stream << "]";
  return stream.str();
}
}  // namespace

class KinematicsNode : public rclcpp::Node
{
public:
  KinematicsNode()
  : Node("six_axis_arm_kinematics_node"),
    current_joints_(Vector6d::Zero())
  {
    target_sub_ = create_subscription<geometry_msgs::msg::PoseStamped>(
      "/target_pose",
      rclcpp::QoS(10),
      std::bind(&KinematicsNode::onTargetPose, this, std::placeholders::_1));

    joint_state_sub_ = create_subscription<sensor_msgs::msg::JointState>(
      "/joint_states",
      rclcpp::QoS(20),
      std::bind(&KinematicsNode::onJointState, this, std::placeholders::_1));

    fk_pub_ = create_publisher<geometry_msgs::msg::PoseStamped>("/fk_pose", 10);
    status_pub_ = create_publisher<std_msgs::msg::String>("/ik_status", 10);

    trajectory_client_ = rclcpp_action::create_client<FollowJointTrajectory>(
      this,
      "/joint_trajectory_controller/follow_joint_trajectory");

    RCLCPP_INFO(get_logger(), "six_axis_arm_kinematics_node is ready");
  }

private:
  void onJointState(const sensor_msgs::msg::JointState::SharedPtr msg)
  {
    std::unordered_map<std::string, double> positions;
    for (std::size_t i = 0; i < msg->name.size() && i < msg->position.size(); ++i) {
      positions[msg->name[i]] = msg->position[i];
    }

    const auto & joint_names = kinematics_.jointNames();
    for (int i = 0; i < 6; ++i) {
      const auto found = positions.find(joint_names[static_cast<std::size_t>(i)]);
      if (found != positions.end()) {
        current_joints_(i) = found->second;
      }
    }
  }

  void onTargetPose(const geometry_msgs::msg::PoseStamped::SharedPtr msg)
  {
    const Eigen::Matrix4d target = poseMsgToMatrix(msg->pose);
    const auto ik = kinematics_.inverse(target, current_joints_);

    std_msgs::msg::String status;
    std::ostringstream status_stream;
    status_stream << ik.message
                  << "; converged=" << (ik.converged ? "true" : "false")
                  << "; joints=" << vectorToString(ik.joints)
                  << "; position_error_m=" << ik.position_error
                  << "; orientation_error_rad=" << ik.orientation_error
                  << "; iterations=" << ik.iterations;
    status.data = status_stream.str();
    status_pub_->publish(status);

    const Eigen::Matrix4d fk = kinematics_.forward(ik.joints);
    geometry_msgs::msg::PoseStamped fk_msg;
    fk_msg.header.stamp = now();
    fk_msg.header.frame_id = "base_link";
    fk_msg.pose = matrixToPoseMsg(fk);
    fk_pub_->publish(fk_msg);

    if (!ik.converged) {
      RCLCPP_WARN(get_logger(), "%s", status.data.c_str());
      return;
    }

    if (!trajectory_client_->wait_for_action_server(std::chrono::seconds(1))) {
      RCLCPP_WARN(
        get_logger(),
        "Trajectory action server is not available; IK result was published but no motion was sent");
      return;
    }

    sendTrajectory(ik.joints);
  }

  void sendTrajectory(const Vector6d & goal_joints)
  {
    FollowJointTrajectory::Goal goal;
    const auto & joint_names = kinematics_.jointNames();
    goal.trajectory.joint_names.assign(joint_names.begin(), joint_names.end());
    goal.trajectory.header.stamp = now();

    constexpr int point_count = 50;
    constexpr double duration = 5.0;
    const auto points = kinematics_.quinticTrajectory(current_joints_, goal_joints, point_count);

    for (int i = 0; i < point_count; ++i) {
      trajectory_msgs::msg::JointTrajectoryPoint point;
      point.positions.assign(points[static_cast<std::size_t>(i)].data(),
        points[static_cast<std::size_t>(i)].data() + 6);

      const double seconds = duration * static_cast<double>(i + 1) /
        static_cast<double>(point_count);
      point.time_from_start = rclcpp::Duration::from_seconds(seconds);
      goal.trajectory.points.push_back(point);
    }

    auto send_options = rclcpp_action::Client<FollowJointTrajectory>::SendGoalOptions();
    send_options.goal_response_callback =
      [this](const GoalHandleFollowJointTrajectory::SharedPtr & goal_handle) {
        if (!goal_handle) {
          RCLCPP_ERROR(get_logger(), "Joint trajectory goal was rejected");
        } else {
          RCLCPP_INFO(get_logger(), "Joint trajectory goal accepted");
        }
      };
    send_options.result_callback =
      [this](const GoalHandleFollowJointTrajectory::WrappedResult & result) {
        if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
          RCLCPP_INFO(get_logger(), "Joint trajectory execution completed");
        } else {
          RCLCPP_WARN(get_logger(), "Joint trajectory execution finished with code %d",
            static_cast<int>(result.code));
        }
      };

    trajectory_client_->async_send_goal(goal, send_options);
  }

  SixAxisArmKinematics kinematics_;
  Vector6d current_joints_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr target_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr fk_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp_action::Client<FollowJointTrajectory>::SharedPtr trajectory_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<KinematicsNode>());
  rclcpp::shutdown();
  return 0;
}

