#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include "robot_arm_kinematics/kinematics.hpp"

namespace
{

struct RunningStats
{
  double sum{0.0};
  double max{0.0};
  int count{0};

  void add(double value)
  {
    sum += value;
    max = std::max(max, value);
    ++count;
  }

  double mean() const
  {
    return count == 0 ? 0.0 : sum / static_cast<double>(count);
  }
};

int parseIntArg(int argc, char ** argv, const std::string & name, int fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      return std::stoi(argv[i + 1]);
    }
  }
  return fallback;
}

double parseDoubleArg(int argc, char ** argv, const std::string & name, double fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == name) {
      return std::stod(argv[i + 1]);
    }
  }
  return fallback;
}

std::uint32_t parseSeedArg(int argc, char ** argv, std::uint32_t fallback)
{
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == "--seed") {
      return static_cast<std::uint32_t>(std::stoul(argv[i + 1]));
    }
  }
  return fallback;
}

}  // namespace

int main(int argc, char ** argv)
{
  const int samples = std::max(1, parseIntArg(argc, argv, "--samples", 1000));
  const std::uint32_t seed = parseSeedArg(argc, argv, 20250624U);
  const double seed_radius_rad =
    std::max(0.0, parseDoubleArg(argc, argv, "--seed-radius", 0.25));

  robot_arm_kinematics::SixAxisArmKinematics kinematics;
  std::mt19937 rng(seed);

  robot_arm_kinematics::Vector6d lower = kinematics.lowerLimits();
  robot_arm_kinematics::Vector6d upper = kinematics.upperLimits();
  std::array<std::uniform_real_distribution<double>, 6> distributions{
    std::uniform_real_distribution<double>(lower(0), upper(0)),
    std::uniform_real_distribution<double>(lower(1), upper(1)),
    std::uniform_real_distribution<double>(lower(2), upper(2)),
    std::uniform_real_distribution<double>(lower(3), upper(3)),
    std::uniform_real_distribution<double>(lower(4), upper(4)),
    std::uniform_real_distribution<double>(lower(5), upper(5))};
  std::uniform_real_distribution<double> seed_offset_distribution(
    -seed_radius_rad, seed_radius_rad);

  RunningStats fk_dq_position;
  RunningStats fk_dq_orientation;
  RunningStats ik_position;
  RunningStats ik_orientation;
  RunningStats ik_iterations;
  int ik_successes = 0;

  for (int sample = 0; sample < samples; ++sample) {
    robot_arm_kinematics::Vector6d q;
    for (int joint = 0; joint < 6; ++joint) {
      q(joint) = distributions[static_cast<std::size_t>(joint)](rng);
    }

    const Eigen::Matrix4d matrix_fk = kinematics.forward(q);
    const Eigen::Matrix4d dq_fk = kinematics.forwardDualQuaternion(q).toTransform();
    fk_dq_position.add(
      (matrix_fk.block<3, 1>(0, 3) - dq_fk.block<3, 1>(0, 3)).norm());
    fk_dq_orientation.add(
      robot_arm_kinematics::SixAxisArmKinematics::orientationErrorNorm(matrix_fk, dq_fk));

    robot_arm_kinematics::Vector6d ik_seed = q;
    for (int joint = 0; joint < 6; ++joint) {
      ik_seed(joint) = std::clamp(
        ik_seed(joint) + seed_offset_distribution(rng),
        lower(joint),
        upper(joint));
    }

    const auto ik = kinematics.inverse(matrix_fk, ik_seed);
    const Eigen::Matrix4d recovered = kinematics.forward(ik.joints);
    const double position_error =
      (matrix_fk.block<3, 1>(0, 3) - recovered.block<3, 1>(0, 3)).norm();
    const double orientation_error =
      robot_arm_kinematics::SixAxisArmKinematics::orientationErrorNorm(matrix_fk, recovered);

    if (ik.converged) {
      ++ik_successes;
      ik_position.add(position_error);
      ik_orientation.add(orientation_error);
      ik_iterations.add(static_cast<double>(ik.iterations));
    }
  }

  Eigen::Matrix4d unreachable = Eigen::Matrix4d::Identity();
  unreachable(0, 3) = 5.0;
  unreachable(1, 3) = 0.0;
  unreachable(2, 3) = 5.0;
  const auto unreachable_ik =
    kinematics.inverse(unreachable, robot_arm_kinematics::Vector6d::Zero());

  std::cout << std::setprecision(10) << std::boolalpha;
  std::cout << "fk_ik_benchmark" << std::endl;
  std::cout << "samples: " << samples << std::endl;
  std::cout << "seed: " << seed << std::endl;
  std::cout << "ik_seed_radius_rad: " << seed_radius_rad << std::endl;
  std::cout << "fk_dq_position_error_mean_m: " << fk_dq_position.mean() << std::endl;
  std::cout << "fk_dq_position_error_max_m: " << fk_dq_position.max << std::endl;
  std::cout << "fk_dq_orientation_error_mean_rad: " << fk_dq_orientation.mean() << std::endl;
  std::cout << "fk_dq_orientation_error_max_rad: " << fk_dq_orientation.max << std::endl;
  std::cout << "ik_success_count: " << ik_successes << std::endl;
  std::cout << "ik_success_rate: " << static_cast<double>(ik_successes) /
    static_cast<double>(samples) << std::endl;
  std::cout << "ik_success_rate_min_required: 0.9" << std::endl;
  std::cout << "ik_position_error_mean_m: " << ik_position.mean() << std::endl;
  std::cout << "ik_position_error_max_m: " << ik_position.max << std::endl;
  std::cout << "ik_orientation_error_mean_rad: " << ik_orientation.mean() << std::endl;
  std::cout << "ik_orientation_error_max_rad: " << ik_orientation.max << std::endl;
  std::cout << "ik_iterations_mean: " << ik_iterations.mean() << std::endl;
  std::cout << "unreachable_target_converged: " << unreachable_ik.converged << std::endl;
  std::cout << "unreachable_position_error_m: " << unreachable_ik.position_error << std::endl;
  std::cout << "unreachable_orientation_error_rad: " << unreachable_ik.orientation_error
            << std::endl;

  const bool fk_dq_ok = fk_dq_position.max < 1.0e-9 && fk_dq_orientation.max < 1.0e-7;
  const bool ik_ok = ik_successes > 0 && ik_position.max < 0.005 &&
    ik_orientation.max < 3.0 * M_PI / 180.0 &&
    static_cast<double>(ik_successes) / static_cast<double>(samples) >= 0.9;
  const bool unreachable_ok = !unreachable_ik.converged;
  return fk_dq_ok && ik_ok && unreachable_ok ? 0 : 1;
}
