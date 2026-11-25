/**
 * @file src/plank_live_layout.h
 * @brief Distinguish an observed display layout from a retained recovery lease.
 */
#pragma once

#include "session/session_context.h"

#include <algorithm>
#include <utility>

namespace plank::topology {
  /** @brief A layout observed on the worker's X server, with independent lease ownership. */
  struct live_layout_t {
    std::string startup_kind;  ///< Legacy startup layout.
    std::string startup_policy;  ///< Configured startup policy.
    std::string kind;  ///< Observed legacy layout.
    bool virtual_layout {};  ///< Whether the observed legacy layout has virtual modes.
    std::vector<std::string> virtual_modes;  ///< Observed legacy mode names.
    bool temporary_physical_lease {};  ///< A temporary lease still owns display recovery.
    uid_t lease_uid {};  ///< Owner of that lease.
    std::optional<plank::session::runtime_display_state_2_t> arrangement;  ///< Arrangement verified against the outputs.
  };

  /**
   * @brief Keep a recorded arrangement's owner while publishing its geometry only when live.
   * @param layout Layout to update for a physical or hybrid startup policy.
   * @param state Supervisor record, which can outlive the arrangement it applied.
   * @param outputs Current X server outputs used in the same topology response.
   * @param applying Whether the supervisor is still applying this arrangement.
   */
  template<class Output>
  void observe_arrangement(live_layout_t &layout,
                           plank::session::runtime_display_state_2_t state,
                           const std::vector<Output> &outputs,
                           bool applying) {
    layout.temporary_physical_lease = true;
    layout.lease_uid = state.lease_uid;
    layout.kind = "physical";
    if (applying || outputs.empty()) return;

    std::size_t shown = 0;
    for (const auto &output : state.outputs) {
      if (output.backing == "off") continue;
      ++shown;
      const auto id = "x11:" + output.randr;
      const auto live = std::find_if(outputs.begin(), outputs.end(), [&](const auto &candidate) {
        return candidate.id == id;
      });
      if (live == outputs.end() || live->x != output.x || live->y != output.y ||
          live->width != output.width || live->height != output.height) {
        return;
      }
    }
    if (shown != outputs.size()) return;

    if (state.origin == "legacy") {
      layout.kind = state.layout;
      layout.virtual_layout = true;
      layout.virtual_modes = {state.mode_1};
      if (!state.mode_2.empty()) layout.virtual_modes.push_back(state.mode_2);
    }
    layout.arrangement = std::move(state);
  }
}  // namespace plank::topology
