/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_SWITCH_H
#define ERIN_SWITCH_H

#include <cstddef>
#include <cassert>

#include "erin_next/erin_next.h"
#include "erin_next/erin_next_const.h"

namespace erin
{
    // Forward Declarations
    struct Model;
    struct SimulationState;

    // Structures
    struct CompId
    {
        // Index into the top-level component array
        uint32_t ComponentIdx;
        // Index into the array of the subtype
        uint32_t SubtypeIdx;
    };

    enum class SwitchState
    {
        Primary = 0,
        Secondary = 1,
    };

    // struct Switch
    // {
    //   size_t InflowConnPrimary;
    //   size_t InflowConnSecondary;
    //   size_t OutflowConn;
    //   flow_t MaxOutflow_W;
    // };

    // Function Declarations
    SwitchState
    SimulationState_GetSwitchState(SimulationState const& ss, CompId const& id);

    // void
    // SimulationState_SetSwitchState(
    //   SimulationState& ss,
    //   CompId const& id,
    //   SwitchState newState
    // );

    // void
    // RunSwitchBackward(
    //   Model& m,
    //   SimulationState& ss,
    //   CompId& id
    // );

    // void
    // RunSwitchForward(
    //   Model& m,
    //   SimulationState& ss,
    //   CompId& id
    // );
}

#endif
