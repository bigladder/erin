/* Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

#ifndef ERIN_RANDOM_H
#define ERIN_RANDOM_H
#include <memory>
#include <random>
#include <vector>

namespace ERIN
{
  ////////////////////////////////////////////////////////////
  // RandomType
  enum class RandomType
  {
    RandomProcess = 0,
    FixedProcess,
    FixedSeries
  };

  ////////////////////////////////////////////////////////////
  // RandomInfo
  class RandomInfo
  {
    public:
      RandomInfo() = default;
      virtual ~RandomInfo() = default;
      RandomInfo(const RandomInfo&) = delete;
      RandomInfo& operator=(const RandomInfo&) = delete;
      RandomInfo(RandomInfo&&) = delete;
      RandomInfo& operator=(RandomInfo&&) = delete;

      virtual std::unique_ptr<RandomInfo> clone() const = 0;
      virtual bool has_seed() const = 0;
      virtual unsigned int get_seed() const = 0;
      virtual RandomType get_type() const = 0;
      virtual double call() = 0;
  };

  bool operator==(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b);
  bool operator!=(
      const std::unique_ptr<RandomInfo>& a,
      const std::unique_ptr<RandomInfo>& b);

  std::unique_ptr<RandomInfo> make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value);

  std::unique_ptr<RandomInfo> make_random_info(
      bool has_fixed_random,
      double fixed_random,
      bool has_seed,
      unsigned int seed_value,
      bool has_fixed_series,
      const std::vector<double>& series);

  ////////////////////////////////////////////////////////////
  // RandomProcess
  class RandomProcess : public RandomInfo
  {
    public:
      RandomProcess();
      RandomProcess(unsigned int seed);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<RandomProcess>(seed);
      };
      [[nodiscard]] bool has_seed() const override { return true; }
      [[nodiscard]] unsigned int get_seed() const override { return seed; }
      [[nodiscard]] RandomType get_type() const override {
        return RandomType::RandomProcess;
      }
      double call() override;

      friend bool operator==(const RandomProcess& a, const RandomProcess& b);

    private:
      unsigned int seed;
      std::mt19937 generator;
      std::uniform_real_distribution<double> distribution;
  };

  bool operator==(const RandomProcess& a, const RandomProcess& b);
  bool operator!=(const RandomProcess& a, const RandomProcess& b);

  ////////////////////////////////////////////////////////////
  // FixedProcess
  class FixedProcess : public RandomInfo
  {
    public:
      FixedProcess(double fixed_value);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<FixedProcess>(fixed_value);
      }
      [[nodiscard]] bool has_seed() const override { return false; }
      [[nodiscard]] unsigned int get_seed() const override { return 0; }
      [[nodiscard]] RandomType get_type() const override { return RandomType::FixedProcess; }
      double call() override { return fixed_value; }
      
      friend bool operator==(const FixedProcess& a, const FixedProcess& b);

    private:
      double fixed_value;
  };

  bool operator==(const FixedProcess& a, const FixedProcess& b);
  bool operator!=(const FixedProcess& a, const FixedProcess& b);

  ////////////////////////////////////////////////////////////
  // FixedSeries
  class FixedSeries : public RandomInfo
  {
    public:
      FixedSeries(const std::vector<double>& series);
      FixedSeries(
          const std::vector<double>& series,
          std::vector<double>::size_type idx);

      std::unique_ptr<RandomInfo> clone() const override {
        return std::make_unique<FixedSeries>(series, idx);
      }
      [[nodiscard]] bool has_seed() const override { return false; }
      [[nodiscard]] unsigned int get_seed() const override { return 0; }
      [[nodiscard]] RandomType get_type() const override {
        return RandomType::FixedSeries;
      }
      double call() override;
      
      friend bool operator==(const FixedSeries& a, const FixedSeries& b);

    private:
      std::vector<double> series;
      std::vector<double>::size_type idx;
      std::vector<double>::size_type max_idx;

      void increment_idx();
  };

  bool operator==(const FixedSeries& a, const FixedSeries& b);
  bool operator!=(const FixedSeries& a, const FixedSeries& b);
}

#endif // ERIN_RANDOM_H
