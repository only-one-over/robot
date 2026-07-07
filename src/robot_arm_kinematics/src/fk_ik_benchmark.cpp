#include <algorithm>
#include <array>
#include <chrono>
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

struct MethodStats
{
  MethodStats(robot_arm_kinematics::IkMethod selected_method)
  : method(selected_method)
  {
  }

  robot_arm_kinematics::IkMethod method;
  RunningStats position;
  RunningStats orientation;
  RunningStats iterations;
  RunningStats runtime_ms;
  int successes{0};
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
  std::array<MethodStats, 3> method_stats{{
    {robot_arm_kinematics::IkMethod::kJacobianTranspose},
    {robot_arm_kinematics::IkMethod::kPseudoinverse},
    {robot_arm_kinematics::IkMethod::kDampedLeastSquares}}};

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

    for (auto & stats : method_stats) {
      const auto start = std::chrono::steady_clock::now();
      const auto ik = kinematics.inverseWithMethod(matrix_fk, ik_seed, stats.method);
      const auto stop = std::chrono::steady_clock::now();
      stats.runtime_ms.add(
        std::chrono::duration<double, std::milli>(stop - start).count());

      if (ik.converged) {
        const Eigen::Matrix4d recovered = kinematics.forward(ik.joints);
        ++stats.successes;
        stats.position.add(
          (matrix_fk.block<3, 1>(0, 3) - recovered.block<3, 1>(0, 3)).norm());
        stats.orientation.add(
          robot_arm_kinematics::SixAxisArmKinematics::orientationErrorNorm(
            matrix_fk, recovered));
        stats.iterations.add(static_cast<double>(ik.iterations));
      }
    }
  }

  Eigen::Matrix4d unreachable = Eigen::Matrix4d::Identity();
  unreachable(0, 3) = 5.0;
  unreachable(1, 3) = 0.0;
  unreachable(2, 3) = 5.0;
  std::array<robot_arm_kinematics::IkResult, 3> unreachable_results;
  for (std::size_t i = 0; i < method_stats.size(); ++i) {
    unreachable_results[i] = kinematics.inverseWithMethod(
      unreachable, robot_arm_kinematics::Vector6d::Zero(), method_stats[i].method);
  }

  std::cout << std::setprecision(10) << std::boolalpha;
  std::cout << "fk_ik_benchmark" << std::endl;
  std::cout << "samples: " << samples << std::endl;
  std::cout << "seed: " << seed << std::endl;
  std::cout << "ik_seed_radius_rad: " << seed_radius_rad << std::endl;
  std::cout << "fk_dq_position_error_mean_m: " << fk_dq_position.mean() << std::endl;
  std::cout << "fk_dq_position_error_max_m: " << fk_dq_position.max << std::endl;
  std::cout << "fk_dq_orientation_error_mean_rad: " << fk_dq_orientation.mean() << std::endl;
  std::cout << "fk_dq_orientation_error_max_rad: " << fk_dq_orientation.max << std::endl;
  for (std::size_t i = 0; i < method_stats.size(); ++i) {
    const auto & stats = method_stats[i];
    const auto & unreachable_result = unreachable_results[i];
    const std::string prefix =
      robot_arm_kinematics::SixAxisArmKinematics::ikMethodName(stats.method);
    std::cout << prefix << "_success_count: " << stats.successes << std::endl;
    std::cout << prefix << "_success_rate: " <<
      static_cast<double>(stats.successes) / static_cast<double>(samples) << std::endl;
    std::cout << prefix << "_position_error_mean_m: " << stats.position.mean() << std::endl;
    std::cout << prefix << "_position_error_max_m: " << stats.position.max << std::endl;
    std::cout << prefix << "_orientation_error_mean_rad: " <<
      stats.orientation.mean() << std::endl;
    std::cout << prefix << "_orientation_error_max_rad: " <<
      stats.orientation.max << std::endl;
    std::cout << prefix << "_iterations_mean: " << stats.iterations.mean() << std::endl;
    std::cout << prefix << "_runtime_mean_ms: " << stats.runtime_ms.mean() << std::endl;
    std::cout << prefix << "_unreachable_converged: " <<
      unreachable_result.converged << std::endl;
    std::cout << prefix << "_unreachable_position_error_m: " <<
      unreachable_result.position_error << std::endl;
    std::cout << prefix << "_unreachable_orientation_error_rad: " <<
      unreachable_result.orientation_error << std::endl;
  }

  const bool fk_dq_ok = fk_dq_position.max < 1.0e-9 && fk_dq_orientation.max < 1.0e-7;
  const auto & dls = method_stats[2];
  const bool ik_ok = dls.successes > 0 && dls.position.max < 0.005 &&
    dls.orientation.max < 3.0 * M_PI / 180.0 &&
    static_cast<double>(dls.successes) / static_cast<double>(samples) >= 0.9;
  const bool unreachable_ok = std::all_of(
    unreachable_results.begin(), unreachable_results.end(),
    [](const auto & result) {return !result.converged;});
  return fk_dq_ok && ik_ok && unreachable_ok ? 0 : 1;
}
