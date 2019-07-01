/* Copyright (c) 2019 Big Ladder Software LLC. All rights reserved.
* See the LICENSE file for additional terms and conditions. */

#ifndef DISCO_DISCO_H
#define DISCO_DISCO_H
#include <string>
#include <vector>
#include "../../vendor/bdevs/include/adevs.h"

namespace DISCO
{
  class Flow
  {
    public:
      Flow(int flowValue);
      int getFlow();

    private:
      int _flow;
  };

  typedef adevs::port_value<Flow> PortValue;

  class Source : public adevs::Atomic<PortValue>
  {
    public:
      static const int PORT_OUT_REQUEST;
      Source();
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& x) override;
      void delta_conf(std::vector<PortValue>& x) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& y) override;
      std::string getResults();
  };

  class Sink : public adevs::Atomic<PortValue>
  {
    public:
      static const int PORT_IN_ACHIEVED;
      static const int PORT_IN_REQUEST;
      Sink();
      void delta_int() override;
      void delta_ext(adevs::Time e, std::vector<PortValue>& x) override;
      void delta_conf(std::vector<PortValue>& x) override;
      adevs::Time ta() override;
      void output_func(std::vector<PortValue>& y) override;
      std::string getResults();
    private:
      int _idx;
      std::vector<int> _times;
      std::vector<int> _loads;
      int _load;
      std::vector<int> _achieved;
  };
}

#endif // DISCO_DISCO_H
