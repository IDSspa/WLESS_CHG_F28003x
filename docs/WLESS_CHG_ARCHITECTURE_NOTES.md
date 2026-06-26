# Wireless Charger Architecture Notes

## SOURCE/SINK Integration Note

This note captures the current working assumptions for the bidirectional
wireless-charger firmware architecture. It should be revisited after the static
HFC tests.

### Operating Roles

The board should expose an application-level role, above the individual BBC and
HFC modes:

```text
SOURCE:
BATTERY -> BBC boost -> DCLINK -> HFC inverter -> coil

SINK:
coil -> HFC rectifier/synchronous stage -> DCLINK -> BBC buck -> BATTERY
```

The role should not be treated as a BBC-only setting. It must coordinate:

- BBC operating mode and gate pattern
- HFC operating mode and phase-shift behavior
- UniPD `tx_1_rx_0` input
- enable sequencing
- protections and current/voltage limits

### UniPD Role Mapping

The UniPD interface already contains:

```text
UNIPD_bbcInputs.tx_1_rx_0
```

Working mapping:

```text
SOURCE -> tx_1_rx_0 = 1
SINK   -> tx_1_rx_0 = 0
```

Current UniPD behavior observed in code:

```text
tx_1_rx_0 = 1 -> phase-shift/HFC command can be enabled
tx_1_rx_0 = 0 -> phase-shift/HFC command is disabled by UniPD
```

This seems coherent with SOURCE acting as transmitter and SINK acting as
receiver, but it must be confirmed against the HFC static tests and the final
power-transfer strategy.

### BBC Duty Semantics

Do not connect UniPD PWM duty outputs directly to the ePWM gate layer until the
physical meaning is verified.

UniPD computes:

```text
UNIPD_bbcOutput.duty_cycle_pwm_a
UNIPD_bbcOutput.duty_cycle_pwm_b
```

The Matlab/C comments indicate these values are derived from:

```text
duty = V_buckboost / V_dc
```

and the limit comments state:

```text
V_L_rif_max = V_bat      -> low-side transistor always closed
V_L_rif_min = V_bat-Vdc  -> high-side transistor always closed
```

This suggests the UniPD duty is closer to a high-side equivalent duty for the
BBC leg, while the safe bench BOOST test currently uses:

```text
BOOST test:
high-side OFF
low-side PWM with real duty
```

Therefore, for SOURCE/BOOST operation, a conversion may be required, possibly:

```text
low_side_boost_duty = 1 - duty_unipd
```

This is only a working hypothesis. It must be validated before enabling UniPD
power outputs.

### BBC Gate Patterns for Test

Bench-tested BOOST pattern:

```text
EPWMxA / high-side = forced low
EPWMxB / low-side  = PWM with commanded duty
```

Required future BUCK test pattern:

```text
EPWMxA / high-side = PWM with commanded duty
EPWMxB / low-side  = forced low
```

The BUCK pattern should be tested with:

```text
DCLINK externally supplied
+BATT replaced by passive load
```

Avoid using a normal bench power supply as a sink unless it is explicitly rated
for reverse current.

### Integration Boundary

Recommended final structure:

```text
Application role/state machine
    -> validates measurements and limits
    -> sets UniPD tx/rx input
    -> runs UniPD controls
    -> adapts UniPD outputs to physical BBC/HFC commands
    -> applies enable sequence and protections
```

The adapter between UniPD and the physical PWM layer should own all conversions
from control-domain quantities to gate-domain quantities. This avoids spreading
SOURCE/SINK assumptions throughout the firmware.

### Open Items

- Verify HFC behavior on the modified board before finalizing SOURCE/SINK state
  transitions.
- Implement and test BBC BUCK dock-test mode.
- Confirm UniPD duty semantics against low-side/high-side gate commands.
- Decide whether `v_ac_rif` should be renamed to an HFC/phase-shift reference in
  a later refactor.
- Define safe enable/disable sequence for SOURCE and SINK.
