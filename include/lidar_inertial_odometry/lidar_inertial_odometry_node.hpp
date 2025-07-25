// Copyright 2024 TIER IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef LIDAR_INERTIAL_ODOMETRY__LIDAR_INERTIAL_ODOMETRY_NODE_HPP_
#define LIDAR_INERTIAL_ODOMETRY__LIDAR_INERTIAL_ODOMETRY_NODE_HPP_

#include "lidar_inertial_odometry/imu_initializer.hpp"
#include "lidar_inertial_odometry/lidar_inertial_odometry.hpp"

#include <pcl_ros/transforms.hpp>
#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <nav_msgs/msg/path.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>

#include <boost/circular_buffer.hpp>

#include <pcl_conversions/pcl_conversions.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tf2/convert.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <execution>

class LidarInertialOdometryNode : public rclcpp::Node
{
public:
  explicit LidarInertialOdometryNode(const rclcpp::NodeOptions & options);
  ~LidarInertialOdometryNode();

  void callback_points(const sensor_msgs::msg::PointCloud2::SharedPtr msg);
  void callback_imu(const sensor_msgs::msg::Imu::SharedPtr msg);
  void callback_initial_pose(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg);
  void callback_map_pose(const geometry_msgs::msg::PoseStamped::SharedPtr msg);

  void publish_local_map(const double stamp);
  void publish_message(
    const sensor_type::Measurement & measurement, const Eigen::Matrix4d & pose,
    const Eigen::Matrix<double, 6, 6> & covariance);

  bool get_transform(
    const std::string & target_frame, const std::string & source_frame,
    geometry_msgs::msg::TransformStamped & transformation);
  void publish_tf(
    const geometry_msgs::msg::Pose pose, const rclcpp::Time stamp, const std::string frame_id,
    const std::string child_frame_id);

  inline rclcpp::Time from_sec(const double timestamp)
  {
    const int seconds = static_cast<int>(timestamp);
    const int nanoseconds = static_cast<int>((timestamp - seconds * 1e9));

    return rclcpp::Time(seconds, nanoseconds);
  }

  inline double to_sec(const std_msgs::msg::Header & header)
  {
    return header.stamp.sec + header.stamp.nanosec / 1e9;
  }

  void process();

private:
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr points_subscriber_;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_subscriber_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
    initial_pose_subscriber_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr map_pose_subscriber_;

  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_stamped_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
    pose_with_covariance_stamped_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr local_map_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr deskew_scan_publisher_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr odometry_path_publisher_;

  rclcpp::TimerBase::SharedPtr timer_;

  tf2_ros::Buffer tf_buffer_{get_clock()};
  tf2_ros::TransformListener tf_listener_{tf_buffer_};
  std::shared_ptr<tf2_ros::TransformBroadcaster> broadcaster_;

  std::thread publisher_thread_;
  std::thread local_map_publisher_thread_;
  std::shared_ptr<LidarInertialOdometry> lio_;

  boost::circular_buffer<Sophus::SE3d> pose_buffer_;

  nav_msgs::msg::Path odometry_path_;

  std::string base_frame_id_;
  std::string map_frame_id_;
};

#endif
