#include <chrono>
#include <cmath>
#include <memory>
#include <string>

#include <Eigen/Dense>

#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "interactive_markers/interactive_marker_server.hpp"
#include "rclcpp/rclcpp.hpp"
#include "visualization_msgs/msg/interactive_marker.hpp"
#include "visualization_msgs/msg/interactive_marker_control.hpp"
#include "visualization_msgs/msg/interactive_marker_feedback.hpp"
#include "visualization_msgs/msg/marker.hpp"

#include "robot_arm_kinematics/kinematics.hpp"

namespace
{
using robot_arm_kinematics::SixAxisArmKinematics;
using robot_arm_kinematics::Vector6d;
using visualization_msgs::msg::InteractiveMarker;
using visualization_msgs::msg::InteractiveMarkerControl;
using visualization_msgs::msg::InteractiveMarkerFeedback;
using visualization_msgs::msg::Marker;

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

void normalizePoseOrientation(geometry_msgs::msg::Pose & pose)
{
  const double norm = std::sqrt(
    pose.orientation.x * pose.orientation.x +
    pose.orientation.y * pose.orientation.y +
    pose.orientation.z * pose.orientation.z +
    pose.orientation.w * pose.orientation.w);
  if (norm < 1.0e-9) {
    pose.orientation.x = 0.0;
    pose.orientation.y = 0.0;
    pose.orientation.z = 0.0;
    pose.orientation.w = 1.0;
    return;
  }
  pose.orientation.x /= norm;
  pose.orientation.y /= norm;
  pose.orientation.z /= norm;
  pose.orientation.w /= norm;
}

InteractiveMarkerControl makeAxisControl(
  const std::string & name,
  int interaction_mode,
  double x,
  double y,
  double z)
{
  InteractiveMarkerControl control;
  control.name = name;
  control.orientation.w = 1.0;
  control.orientation.x = x;
  control.orientation.y = y;
  control.orientation.z = z;
  control.interaction_mode = interaction_mode;
  return control;
}

InteractiveMarkerControl makeVisualControl()
{
  Marker sphere;
  sphere.type = Marker::SPHERE;
  sphere.scale.x = 0.07;
  sphere.scale.y = 0.07;
  sphere.scale.z = 0.07;
  sphere.color.r = 0.95F;
  sphere.color.g = 0.34F;
  sphere.color.b = 0.12F;
  sphere.color.a = 0.92F;

  Marker x_axis;
  x_axis.type = Marker::ARROW;
  x_axis.pose.orientation.w = 1.0;
  x_axis.scale.x = 0.16;
  x_axis.scale.y = 0.015;
  x_axis.scale.z = 0.015;
  x_axis.color.r = 0.85F;
  x_axis.color.g = 0.08F;
  x_axis.color.b = 0.08F;
  x_axis.color.a = 0.9F;

  Marker y_axis = x_axis;
  y_axis.pose.orientation.z = 0.70710678;
  y_axis.pose.orientation.w = 0.70710678;
  y_axis.color.r = 0.05F;
  y_axis.color.g = 0.55F;
  y_axis.color.b = 0.12F;

  Marker z_axis = x_axis;
  z_axis.pose.orientation.y = -0.70710678;
  z_axis.pose.orientation.w = 0.70710678;
  z_axis.color.r = 0.08F;
  z_axis.color.g = 0.22F;
  z_axis.color.b = 0.9F;

  InteractiveMarkerControl control;
  control.name = "target_visual";
  control.always_visible = true;
  control.interaction_mode = InteractiveMarkerControl::NONE;
  control.markers.push_back(sphere);
  control.markers.push_back(x_axis);
  control.markers.push_back(y_axis);
  control.markers.push_back(z_axis);
  return control;
}
}  // namespace

class TargetMarkerNode : public rclcpp::Node
{
public:
  TargetMarkerNode()
  : Node("target_marker_node")
  {
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
    marker_namespace_ = declare_parameter<std::string>("marker_namespace", "target_marker");
    publish_topic_ = declare_parameter<std::string>("publish_topic", "/target_pose");
    publish_while_dragging_ = declare_parameter<bool>("publish_while_dragging", false);
    drag_publish_period_ = std::chrono::duration<double>(
      declare_parameter<double>("drag_publish_period", 0.35));

    target_pub_ = create_publisher<geometry_msgs::msg::PoseStamped>(publish_topic_, 10);
    marker_server_ = std::make_unique<interactive_markers::InteractiveMarkerServer>(
      marker_namespace_,
      get_node_base_interface(),
      get_node_clock_interface(),
      get_node_logging_interface(),
      get_node_topics_interface(),
      get_node_services_interface());

    createMarker();
    publishTarget(current_pose_, "initial");
    RCLCPP_INFO(
      get_logger(),
      "target_marker_node ready: add RViz InteractiveMarkers display on /%s/update",
      marker_namespace_.c_str());
  }

private:
  void createMarker()
  {
    Vector6d q;
    q << 0.2, 0.35, -0.55, 0.75, -0.35, 0.25;
    current_pose_ = matrixToPoseMsg(kinematics_.forward(q));

    InteractiveMarker marker;
    marker.header.frame_id = base_frame_;
    marker.name = marker_name_;
    marker.description = "Drag tool0 target";
    marker.scale = 0.22;
    marker.pose = current_pose_;

    marker.controls.push_back(makeVisualControl());
    marker.controls.push_back(makeAxisControl(
      "move_x",
      InteractiveMarkerControl::MOVE_AXIS,
      1.0,
      0.0,
      0.0));
    marker.controls.push_back(makeAxisControl(
      "rotate_x",
      InteractiveMarkerControl::ROTATE_AXIS,
      1.0,
      0.0,
      0.0));
    marker.controls.push_back(makeAxisControl(
      "move_y",
      InteractiveMarkerControl::MOVE_AXIS,
      0.0,
      1.0,
      0.0));
    marker.controls.push_back(makeAxisControl(
      "rotate_y",
      InteractiveMarkerControl::ROTATE_AXIS,
      0.0,
      1.0,
      0.0));
    marker.controls.push_back(makeAxisControl(
      "move_z",
      InteractiveMarkerControl::MOVE_AXIS,
      0.0,
      0.0,
      1.0));
    marker.controls.push_back(makeAxisControl(
      "rotate_z",
      InteractiveMarkerControl::ROTATE_AXIS,
      0.0,
      0.0,
      1.0));

    marker_server_->insert(
      marker,
      std::bind(&TargetMarkerNode::processFeedback, this, std::placeholders::_1));
    marker_server_->applyChanges();
  }

  void processFeedback(const InteractiveMarkerFeedback::ConstSharedPtr feedback)
  {
    if (feedback->marker_name != marker_name_) {
      return;
    }

    current_pose_ = feedback->pose;
    normalizePoseOrientation(current_pose_);

    if (feedback->event_type == InteractiveMarkerFeedback::MOUSE_UP) {
      publishTarget(current_pose_, "mouse_up");
      return;
    }

    if (publish_while_dragging_ &&
      feedback->event_type == InteractiveMarkerFeedback::POSE_UPDATE)
    {
      const auto now_time = now();
      if ((now_time - last_drag_publish_time_).seconds() >= drag_publish_period_.count()) {
        publishTarget(current_pose_, "drag");
        last_drag_publish_time_ = now_time;
      }
    }
  }

  void publishTarget(const geometry_msgs::msg::Pose & pose, const std::string & reason)
  {
    geometry_msgs::msg::PoseStamped msg;
    msg.header.stamp = now();
    msg.header.frame_id = base_frame_;
    msg.pose = pose;
    target_pub_->publish(msg);
    RCLCPP_INFO(
      get_logger(),
      "Published %s target pose at x=%.3f y=%.3f z=%.3f",
      reason.c_str(),
      pose.position.x,
      pose.position.y,
      pose.position.z);
  }

  SixAxisArmKinematics kinematics_;
  std::unique_ptr<interactive_markers::InteractiveMarkerServer> marker_server_;
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr target_pub_;
  geometry_msgs::msg::Pose current_pose_;
  rclcpp::Time last_drag_publish_time_{0, 0, RCL_ROS_TIME};
  std::chrono::duration<double> drag_publish_period_{0.35};
  std::string base_frame_;
  std::string marker_namespace_;
  std::string publish_topic_;
  const std::string marker_name_{"tool0_target"};
  bool publish_while_dragging_{false};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TargetMarkerNode>());
  rclcpp::shutdown();
  return 0;
}
