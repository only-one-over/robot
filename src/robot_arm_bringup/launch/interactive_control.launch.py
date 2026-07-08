from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    sim_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare("robot_arm_bringup"),
                "launch",
                "sim.launch.py",
            ])
        ])
    )

    rviz_config = PathJoinSubstitution([
        FindPackageShare("robot_arm_description"),
        "rviz",
        "six_axis_arm.rviz",
    ])

    target_marker = Node(
        package="robot_arm_kinematics",
        executable="target_marker_node",
        output="screen",
        parameters=[{
            "base_frame": "base_link",
            "marker_namespace": "target_marker",
            "publish_topic": "/target_pose",
            "publish_while_dragging": False,
            "drag_publish_period": 0.35,
        }],
    )

    rviz = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2_interactive_control",
        output="screen",
        arguments=["-d", rviz_config],
    )

    return LaunchDescription([
        sim_launch,
        TimerAction(period=6.0, actions=[target_marker]),
        TimerAction(period=7.0, actions=[rviz]),
    ])
