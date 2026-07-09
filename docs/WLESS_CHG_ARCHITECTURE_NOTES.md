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

Project DT Rev. 1.2 confirms the same high-level split. In the document:

- the connector acting as `SOURCE` runs source-side bus/battery current
  controllers, BBC boost duty controllers, and an HFC/coil-current controller;
- the connector acting as `LOAD` runs load-side bus/battery current controllers
  and BBC buck duty controllers;
- the HFC command is expressed as phase-shift references while the BBC command
  is expressed as duty references for the two BBC branches.

This reinforces the decision to implement SOURCE/SINK as an application-level
role and not as a local BBC-only flag.

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

Working hypothesis for the future BUCK bench-test pattern:

```text
EPWMxA / high-side = PWM with commanded duty
EPWMxB / low-side  = forced low
```

This is a conservative non-synchronous BUCK pattern for low-energy bench tests.
It is inferred from the UniPD duty semantics (`duty = V_buckboost / V_dc`) and
from the BBC topology, but it is not yet explicitly confirmed by the technical
document. Confirm with the technical lead and verify on the docking station
before applying it to the modified power board.

Firmware status: this pattern is implemented only in the BBC dock-test hook.
It is not connected to UniPD power outputs and it is disabled unless
`TTPLPFC_bbcDockTestEnable` is explicitly set at runtime.

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

### TI Legacy Reuse Map

The original TI firmware contains useful bidirectional CLLLC logic, but it does
not contain a ready-to-use BBC buck controller for the modified wireless-charger
hardware.

Useful legacy CLLLC/HFC items for future SINK work:

| Legacy item | Original role | Possible WLESS use | Reuse level |
| --- | --- | --- | --- |
| `CLLLC_POWER_FLOW_PRIM_SEC` / `CLLLC_POWER_FLOW_SEC_PRIM` | Compile-time direction selection for CLLLC power flow | Basis for SOURCE/SINK HFC direction semantics | Conceptual reuse |
| `ISR2_secToPrimPowerFlow()` | Secondary-to-primary CLLLC ISR path | Reference for receiver-side HFC behavior | Partial reuse |
| `CLLLC_calculatePWMDutyPeriodPhaseShiftTicks_secToPrimPowerFlow()` | Calculates PWM timing for reverse CLLLC flow | Reference for reverse HFC timing and phase-shift update | Partial reuse |
| `CLLLC_changeSynchronousRectifierPwmBehavior()` | Runtime switch between synchronous-rectifier states | Candidate pattern for clean HFC enable/disable and rectifier mode selection | Partial reuse |
| `CLLLC_HAL_setupSynchronousRectificationAction()` | CMPSS/XBAR/trip configuration for active synchronous rectification | Reference for current-zero-crossing based HFC rectifier commutation | Conceptual/partial reuse |
| `CLLLC_HAL_setupSynchronousRectificationActionDebug()` | Exposes comparator/debug signals on output XBAR | Useful diagnostic pattern during receiver HFC bring-up | Partial reuse |

Important limitations:

- The original synchronous-rectifier code targets the old CLLLC secondary bridge
  on `EPWM3/EPWM4`; those MOSFETs are removed in the modified board.
- In the WLESS board each side uses the retained HFC bridge (`EPWM1/EPWM2`) for
  inverter/rectifier behavior. Any legacy rectifier logic must therefore be
  remapped to the local HFC bridge of the board acting as SINK.
- The original CMPSS paths use legacy primary/secondary tank-current signals.
  The WLESS receiver path must be checked against the actual coil/tank current
  measurements available on the modified board.
- `CLLLC_pwmSwStateActive` is not a reliable runtime indication in the current
  `CLLLC_SECONDARY_ENABLED = 0` configuration, because the secondary behavior
  update path is intentionally not active.

TTPLPFC/PFC legacy findings:

- The original TTPLPFC code is a totem-pole PFC/boost implementation, not a
  battery-side buck converter.
- The legacy PFC code does contain useful comments and patterns about
  synchronous-FET duty polarity. In particular, the original code notes that the
  duty command can refer to the synchronous FET, so a low active-FET duty may
  require writing a high duty command.
- This is useful background for interpreting TI gate polarity, but it is not a
  directly reusable BBC buck algorithm.

Current conclusion:

```text
HFC rectifier / synchronous rectifier:
    use TI CLLLC reverse-flow code as a reference, then remap and simplify.

BBC buck:
    keep the WLESS dock-test implementation as the current low-energy test
    path; final control still needs a dedicated SOURCE/SINK adapter.
```

### Open Items

- Verify HFC behavior on the modified board before finalizing SOURCE/SINK state
  transitions.
- BBC BUCK dock-test mode has been implemented and bench-tested at low voltage;
  final UniPD-to-gate integration is still open.
- Confirm UniPD duty semantics against low-side/high-side gate commands.
- Define the SINK HFC rectifier strategy, starting from the TI legacy CLLLC
  synchronous-rectification code but remapping it to the retained HFC bridge.
- Keep legacy SFRA disabled. The TI SFRA GUI path can configure SCIA on
  GPIO28/GPIO29; those pins are reserved for the future application
  communication feature.
- Decide whether `v_ac_rif` should be renamed to an HFC/phase-shift reference in
  a later refactor.
- Define safe enable/disable sequence for SOURCE and SINK.
