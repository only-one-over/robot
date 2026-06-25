cmake_minimum_required(VERSION 3.20)
project(robot_arm_kinematics)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(control_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(interactive_markers REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

add_library(${PROJECT_NAME}
  src/kinematics.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)

add_executable(kinematics_node src/kinematics_node.cpp)
target_link_libraries(kinematics_node ${PROJECT_NAME})
ament_target_dependencies(kinematics_node
  control_msgs
  geometry_msgs
  rclcpp
  rclcpp_action
  sensor_msgs
  std_msgs
  trajectory_msgs
)

add_executable(move_to_pose_demo src/move_to_pose_demo.cpp)
target_link_libraries(move_to_pose_demo ${PROJECT_NAME})
ament_target_dependencies(move_to_pose_demo
  geometry_msgs
  rclcpp
)

add_executable(fk_ik_test src/fk_ik_test.cpp)
target_link_libraries(fk_ik_test ${PROJECT_NAME})
ament_target_dependencies(fk_ik_test rclcpp)

add_executable(fk_ik_benchmark src/fk_ik_benchmark.cpp)
target_link_libraries(fk_ik_benchmark ${PROJECT_NAME})

add_executable(target_marker_node src/target_marker_node.cpp)
target_link_libraries(target_marker_node ${PROJECT_NAME})
ament_target_dependencies(target_marker_node
  geometry_msgs
  interactive_markers
  rclcpp
  visualization_msgs
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  TARGETS
    kinematics_node
    move_to_pose_demo
    fk_ik_test
    fk_ik_benchmark
    target_marker_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_dependencies(Eigen3)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)

ament_package()
