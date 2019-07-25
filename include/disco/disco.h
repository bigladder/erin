/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <vector>
#include "../../vendor/bdevs/include/adevs.h"

namespace DISCO
{
  ////////////////////////////////////////////////////////////
  // Utility Functions
  int clamp_toward_0(int value, int lower, int upper);

  ////////////////////////////////////////////////////////////
  // Flow
  class Flow
  {
    public:
      Flow(int flowValue);
      int getFlow();

    private:
      int _flow;
  };

  typedef adevs::port_value<Flow> PortValue;

  ////////////////////////////////////////////////////////////
  // Source
  class Source : public adevs::Atomic<PortValue>
  {
    public:
      static const int PORT_OUT_REQUEST;
      Source();
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      std::string getResults();

    private:
      int _time;
      std::vector<int> _times;
      std::vector<int> _loads;
  };

  ////////////////////////////////////////////////////////////
  // FlowLimits
  class FlowLimits : public adevs::Atomic<PortValue>
  {
    public:
      static const int port_input_request;
      static const int port_output_request;
      static const int port_input_achieved;
      static const int port_output_achieved;
      //FlowLimits(int upperLimit);
      FlowLimits(int lower_limit, int upper_limit);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;

    private:
      int lower_limit;
      int upper_limit;
      bool report_input_request;
      bool report_output_achieved;
      int input_request;
      int output_achieved;
  };

  ////////////////////////////////////////////////////////////
  // Sink
  class Sink : public adevs::Atomic<PortValue>
  {
    public:
      static const int PORT_IN_ACHIEVED;
      static const int PORT_IN_REQUEST;
      Sink();
      Sink(std::vector<int> times, std::vector<int> loads);
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& xs) override;
      void delta_conf(std::vector<PortValue>& xs) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& ys) override;
      std::string getResults();

    private:
      int _idx;
      std::vector<int> _times;
      std::vector<int> _loads;
      int _time;
      int _load;
      std::vector<int> _achieved_times;
      std::vector<int> _achieved_loads;
  };
}

#endif // DISCO_DISCO_H
