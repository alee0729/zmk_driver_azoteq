# ZMK Driver for Azoteq IQS5XX Trackpads

Note: This driver was originally developed by @stelmakhdigital and I wanted to add some features/bug fixes.  But I don't read Russian, so I asked my AI buddy to make this English [translated](https://github.com/geeksville/zmk_driver_azoteq/commit/4c56195658bbf31851b9892c63f758f50ea48d4d) version.  I hope you find it useful.  It now seems to work well for me.  If you see problems please open issues and I'll do what I can to help.

## Compatibility

This driver should work with any IQS5XX-based trackpad (TPS43 or TPS65).

## Features

- Trackpad movement, with optional macOS-like **pointer acceleration** (`pointer-acceleration`): cursor gain scales with finger speed (slow = fine control, fast = more reach). Disabled by default (purely linear) for backward compatibility.
- Smooth 2-finger scrolling: both axes are emitted (fluid/diagonal) with a per-axis sub-detent remainder carry, so slow scrolling accumulates instead of truncating to a notchy stop.
- Robust event delivery: the driver runs on its own dedicated workqueue, so a transient back-pressure on the input event queue (e.g. forwarding a gesture burst over a BLE split) can never stall system-wide work. Gesture bursts (zoom) are also bounded per report. Latched buttons/drags are always released on suspend, I2C errors, and chip resets so a mouse button is never left stuck.
- Touch reporting: finger contact is reported as `INPUT_BTN_TOUCH` (1 when one or more fingers are on the pad, 0 when all fingers are lifted).
- Single tap: registered as left click.
- Two-finger tap: registered as right click.
- Press and hold: registered as continuous left click (drag).
- Vertical and horizontal scrolling - Modern 'gesture based' two finger swiping
- 1-finger left/right/up/down swipes send INPUT_BTN_WEST/EAST/NORTH/SOUTH events.  You can map them to other keys/actions as you wish in your dts file.
- 3-finger swipes (all four directions) are detected separately - latched and threshold-based, so each swipe fires exactly once - and send their own distinct, configurable codes (defaults INPUT_BTN_6/7/8/9 for up/down/left/right). Because they differ from the 1-finger swipe codes, the host can tell them apart. Tune with `three-finger-swipe-threshold`.
- Pinch-to-zoom sends distinct, configurable momentary codes for zoom-in (pinch-out) vs zoom-out (pinch-in), defaults INPUT_BTN_4 / INPUT_BTN_5, stepped via `zoom-step` so it does not flood events. Map them in your ZMK config (for example to Ctrl+= / Ctrl+-).

## Usage

- In the configuration file (for the appropriate half), add `CONFIG_INPUT_TPS43` to enable the driver

```
CONFIG_INPUT_TPS43=y
```

- In the `.overlay` file, specify `compatible = "azoteq,tps43"` inside the i2c node where the trackpad will be used (example below).

> For complete trackpad configuration information, see the file: [available trackpad settings](./dts/bindings/input/azoteq,tps43-common.yaml)

```
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;
    pinctrl-0 = <&i2c0_default>;  /* Configuration for SDA and SCL */
    pinctrl-1 = <&i2c0_sleep>;    /* Configuration for SDA and SCL */
    pinctrl-names = "default", "sleep";

    tps43_trackpad: trackpad@74 {
        compatible = "azoteq,tps43";
        reg = <0x74>;
        status = "okay";
        
        /* GPIO connections */
        rdy-gpios = <&pro_micro 21 GPIO_ACTIVE_HIGH>;  /* RDY pin */
        rst-gpios = <&pro_micro 20 GPIO_ACTIVE_HIGH>;  /* RST pin */

        enable-power-management;
        
        sensitivity = <50>;            /* 50% of raw sensor speed (driver default). Lower = slower cursor.
                                          Sub-unit remainder is carried between reports so slow moves still register.
                                          For a bigger change, also lower x-resolution/y-resolution below. */
        scroll-sensitivity = <50>;     /* 50% = normal state */
        zoom-sensitivity = <50>;       /* 50% = normal state */

        /* Optional macOS-like pointer acceleration (gain scales with finger speed). */
        // pointer-acceleration;
        // accel-min-gain = <60>;         /* % gain at/below the slow threshold (fine control) */
        // accel-max-gain = <240>;        /* % gain at/above the fast threshold (fast flicks) */
        // accel-slow-threshold = <3>;    /* per-report raw speed |dx|+|dy| for min gain */
        // accel-fast-threshold = <20>;   /* per-report raw speed |dx|+|dy| for max gain */

        /* Report rate (ms): a fast active rate is the biggest smoothness lever. */
        // report-rate-active = <10>;        /* 100 Hz while tracking */
        // report-rate-idle-touch = <16>;    /* resting-finger rate */

        /* Tap / hold timing (ms): shorter = snappier click and faster drag engage. */
        // tap-time = <180>;
        // hold-time = <300>;

        filter-settings=<0x0B>;        /* See filter description in `available settings` */
        // filter-dynamic-bottom=<7>;     /* Dynamic filter bottom beta (optional) */
        // filter-dynamic-lower=<6>;      /* Dynamic filter lower speed (optional) */
        // filter-dynamic-upper=<0xFA>;   /* Dynamic filter upper speed (optional) */
        x-resolution=<2048>;            /* X resolution in pixels */
        y-resolution=<1792>;            /* Y resolution in pixels */

        // swipe-initial-distance=<500>;          /* Swipe initial distance in px (optional) */
        // swipe-initial-time=<200>;              /* Swipe initial time in ms (optional) */
        // swipe-angle=<20>;                      /* Max swipe angle in degrees (optional) */
        // swipe-consecutive-distance=<200>;      /* Swipe consecutive distance in px (optional) */
        // swipe-consecutive-time=<150>;          /* Swipe consecutive time in ms (optional) */
        // scroll-initial-distance=<100>;         /* Scroll initial distance in px (optional) */
        // scroll-angle=<30>;                     /* Max scroll angle in degrees (optional) */
        // zoom-initial-distance=<100>;           /* Zoom initial distance in px (optional) */
        // zoom-consecutive-distance=<50>;        /* Zoom consecutive distance in px (optional) */

        /* Gesture output codes (optional - defaults shown). Map these in your ZMK config.
           To use the INPUT_BTN_* names, #include <zephyr/dt-bindings/input/input-event-codes.h>. */
        // zoom-in-code=<INPUT_BTN_4>;            /* pinch-out (zoom in) */
        // zoom-out-code=<INPUT_BTN_5>;           /* pinch-in (zoom out) */
        // zoom-step=<30>;                        /* accumulated magnitude per zoom event */
        // three-finger-swipe-up-code=<INPUT_BTN_6>;
        // three-finger-swipe-down-code=<INPUT_BTN_7>;
        // three-finger-swipe-left-code=<INPUT_BTN_8>;
        // three-finger-swipe-right-code=<INPUT_BTN_9>;
        // three-finger-swipe-threshold=<150>;    /* travel (raw units) to fire a 3-finger swipe */

        scroll;
        two-finger-tap;
        single-tap;
        press-and-hold;
        swipes;
        zoom;

        switch-xy;
        invert-scroll-y;
    };
};
```

- Now you need to configure a listener for tracking touches:

> Important: If your trackpad `.overlay` configuration is on the central device, use `Option 1`. If the trackpad is located on the peripheral half of the keyboard, use `Option 2`

---
**Option 1**

Simply specify the listener on the central device itself.

```
/ {
    tps43_input: tps43_input {
        compatible = "zmk,input-listener";
        device = <&tps43_trackpad>;
    };
};
```

---

... Otherwise ...

---

**Option 2**

Configure `split_inputs` where the trackpad is used (on the peripheral half of the keyboard)

```
/ {
    split_inputs {
        #address-cells = <1>;
        #size-cells = <0>;

        tps43_split: tps43_split@0 {
            compatible = "zmk,input-split";
            reg = <0>;
            device = <&tps43_trackpad>;
        };
    };
};
```

Now on the central part, you need to specify a listener but without specifying `device` (device is specified where the trackpad is used - on the peripheral part)

```
/ {
    split_inputs {
        #address-cells = <1>;
        #size-cells = <0>;

        tps43_split: tps43_split@0 {
            compatible = "zmk,input-split";
            reg = <0>;
            /* No device property here - this is a proxy on the central side */
        };
    };

    tps43_listener: tps43_listener {
        compatible = "zmk,input-listener";
        device = <&tps43_split>;
        status = "okay";
    };
};
```
---


> Configuring the Azoteq trackpad requires 5 pins!

Power:
3V on nice!nano -> VDD on IQS5xx.
GND (Ground) on nice!nano -> GND on IQS5xx.

I2C signals:
SDA on nice!nano -> SDA on IQS5xx.
SCL on nice!nano -> SCL on IQS5xx.

"DR" or "RDY" pin on IQS5xx -> Any available GPIO on nice!nano. In the devicetree, this pin is specified as rdy-gpios.
"RST" pin is used to initialize device reset. In the devicetree, this pin is specified as rst-gpios.


## Proper Driver Operation Sequence

```txt
1. Power on / Hardware reset
   └─> Wait 10ms
   └─> RST: LOW (10ms) → HIGH
   └─> Wait ~600ms for firmware loading

2. Check SHOW_RESET flag (0x000F bit 0)
   └─> Poll until flag appears

3. Acknowledge reset
   └─> Write ACK_RESET (0x0431 = 0x80)

4. Device configuration
   └─> System Config 1 (0x058F) - event modes
   └─> XY Config (0x0669) - axis settings
   └─> Filter settings, gestures, etc.

5. Complete setup
   └─> Write SETUP_COMPLETE (0x058E = 0x40)

6. Configure GPIO interrupt (RDY)
   └─> AFTER full configuration

```

## Power Management

The driver supports two independent power management mechanisms to reduce power consumption:

### Integration with ZMK Power Management System

The trackpad automatically enters sleep mode when the keyboard transitions to idle/sleep state.

**How it works:**
- The `tps43_idle_sleeper.c` module subscribes to ZMK `zmk_activity_state_changed` events
- When ZMK transitions to `SLEEP` state, the trackpad is always fully suspended
- When ZMK transitions to `IDLE` state, the behavior is chosen per device (see below)
- When returning to `ACTIVE` state, the trackpad resumes from suspend and/or its
  normal scan rate is restored

**IDLE behavior — three options:**

| Configuration | On ZMK IDLE | Idle power | Wake |
|---|---|---|---|
| *(neither property)* | nothing; the chip's internal auto-power ladder (Active → Idle → LP1 → LP2) is the only mechanism | highest | touch, instant (≤ LP2 scan period) |
| `idle-scan-rate-ms = <500>` | the LP2 ALP scan is slowed to the given period (ms); the pad keeps sensing | low | touch — the pad wakes itself; first touch registers within ~1 scan period |
| `idle-sleep` | full suspend: charge transfer halts, RDY is disabled | lowest (~µA) | **other keyboard activity only** (e.g. a key press on the same half); a suspended pad cannot see touches |

`idle-scan-rate-ms` and `idle-sleep` are mutually exclusive (compile-time error).
Both require `enable-power-management`.

```
&i2c0 {
    tps43_trackpad: trackpad@74 {
        compatible = "azoteq,tps43";
        /* ... */
        enable-power-management;
        idle-scan-rate-ms = <500>;  /* touch-to-wake slow scan while ZMK is idle */
        /* OR: idle-sleep;             lowest power, but touch cannot wake it */
    };
};
```

**Important:** This mechanism only works if `enable-power-management` is enabled.

### Technical Details

**Suspend (SLEEP state, and IDLE with `idle-sleep`):**
- Controlled via the `SYSTEM_CONTROL_1` register (0x0432)
- The `TPS43_SUSPEND` bit (BIT(0)) is set to enter suspend mode
- In suspend mode, the trackpad consumes minimal power and does not process
  touches; the RDY interrupt is disabled until resume
- Waking requires another ZMK activity source (the trackpad cannot wake itself)

**Slow-scan idle (IDLE with `idle-scan-rate-ms`):**
- The driver rewrites `REPORT_RATE_LP2` (0x0582) to the configured period on
  IDLE and restores the normal rate (the `report-rate-lp2` value if set, else
  the firmware default read back once at configure time) on ACTIVE
- The RDY interrupt stays armed: the chip keeps sensing on its ALP channel and
  wakes itself into Active mode on touch, so the first touch both registers and
  restores full responsiveness

> Without `enable-power-management`, power management is completely disabled

