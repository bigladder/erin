/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#include "erin/switch.h"

namespace erin
{
    SwitchState
    SimulationState_GetSwitchState(
        SimulationState const& ss,
        CompId const& switchId
    )
    {
        assert(switchId.SubtypeIdx < ss.SwitchStates.size());
        return ss.SwitchStates[switchId.SubtypeIdx];
    }
}
