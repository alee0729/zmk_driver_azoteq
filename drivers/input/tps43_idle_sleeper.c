/**
* @file tps43_idle_sleeper.c
* @brief Integration of TPS43 trackpad power management with ZMK power management system
*
* This module subscribes to ZMK activity state change events and reduces trackpad
* power per the device's configuration:
* - SLEEP always fully suspends the trackpad (sensing halts, ~uA).
* - IDLE either suspends (`idle-sleep`), slows the LP2 ALP scan so the pad keeps
*   sensing and wakes itself on touch (`idle-scan-rate-ms`), or does nothing and
*   leaves power to the chip's internal auto-power ladder (default).
* - ACTIVE resumes from suspend and restores the normal scan rate.
*
* Works in conjunction with automatic power management in the main driver (tps43.c),
* which monitors trackpad idle time.
*/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include "tps43.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tps43_sleeper, CONFIG_INPUT_LOG_LEVEL);

/**
* @brief Macro to get device pointer from devicetree
*/
#define GET_TPS43_DEV(node_id) DEVICE_DT_GET(node_id),

/**
* @brief Array of pointers to all TPS43 devices in the system
* 
* Automatically populated from devicetree for all devices with azoteq_tps43 compatibility
*/
static const struct device *tps43_devs[] = {DT_FOREACH_STATUS_OKAY(azoteq_tps43, GET_TPS43_DEV)};

/**
* @brief Handler for ZMK activity state change event
* 
* This function is called when the keyboard activity state changes:
* - ZMK_ACTIVITY_ACTIVE - keyboard is active, trackpad should be awakened
 * - ZMK_ACTIVITY_IDLE - keyboard is idle, trackpad is put to sleep only if
 *   the device has the `idle-sleep` property set
* - ZMK_ACTIVITY_SLEEP - keyboard is sleeping, trackpad is put to sleep
* 
* @param eh Pointer to activity state change event
* @return 0 on successful handling
*/
static int on_activity_state(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *state_ev = as_zmk_activity_state_changed(eh);
    if (!state_ev) {
        LOG_WRN("Event not found, ignoring");
        return 0;
    }

    // Apply change to all TPS43 devices in the system
    for (size_t i = 0; i < ARRAY_SIZE(tps43_devs); i++) {
        const struct device *dev = tps43_devs[i];
        const struct tps43_config *config = dev->config;
        int ret = 0;

        switch (state_ev->state) {
        case ZMK_ACTIVITY_SLEEP:
            // Deep sleep always fully suspends the trackpad.
            LOG_INF("ZMK activity state change: %d -> trackpad %zu sleep",
                    state_ev->state, i);
            ret = tps43_set_sleep(dev, true);
            break;

        case ZMK_ACTIVITY_IDLE:
            /*
             * IDLE behavior is opt-in per device: `idle-sleep` fully suspends
             * (lowest power, but the pad can't wake itself - sensing halts);
             * `idle-scan-rate-ms` slows the LP2 ALP scan so the pad keeps
             * sensing and wakes itself on touch. With neither, the chip's
             * internal auto-power ladder is the only idle mechanism.
             */
            if (config->idle_sleep) {
                LOG_INF("ZMK activity state change: %d -> trackpad %zu sleep",
                        state_ev->state, i);
                ret = tps43_set_sleep(dev, true);
            } else if (config->idle_scan_rate_ms != -1) {
                LOG_INF("ZMK activity state change: %d -> trackpad %zu slow-scan idle",
                        state_ev->state, i);
                ret = tps43_set_idle_scan(dev, true);
            }
            break;

        default: /* ZMK_ACTIVITY_ACTIVE */
            // Resume from suspend first: a suspended chip can't take the scan
            // rate write, so the restore below must come after the wake-up.
            LOG_INF("ZMK activity state change: %d -> trackpad %zu active",
                    state_ev->state, i);
            ret = tps43_set_sleep(dev, false);
            if (ret == 0) {
                ret = tps43_set_idle_scan(dev, false);
            }
            break;
        }

        if (ret != 0) {
            LOG_WRN("Trackpad power management error %zu: %d", i, ret);
        }
    }

    return 0;
}

// Register ZMK event listener
ZMK_LISTENER(tps43_idle_sleeper, on_activity_state);
// Subscribe to activity state change events
ZMK_SUBSCRIPTION(tps43_idle_sleeper, zmk_activity_state_changed);