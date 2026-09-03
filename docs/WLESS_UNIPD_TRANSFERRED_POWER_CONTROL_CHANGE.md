# UniPD transferred-power control import

## Scope

This change imports the two outer WPT power-transfer loops found in:

- `docs/UniPd/Controllo_Sistema_WPT_funzione_v14.m`;
- `docs/UniPd/p_schema_completo_generale_v2.m`.

It is a functional software change, not a test result. No entry is added to the
test report until the imported functions are exercised through the UART test
interface and then reviewed for hardware activation.

## Imported functions

`UNIPD_controlRxDcBusViaTransferredPower()` implements Matlab
`Controllo_Tensione_Bus_dc_I_coil_loc()`:

1. calculate `Vdc_ref^2 - Vdc^2`;
2. generate the requested transferred power through the discrete PI;
3. clamp it between zero and the transmitter-advertised power limit;
4. calculate `Icoil_ref = pi/2 * Ptrasf_ref / Vdc`;
5. return `Icoil_ref - Icoil_local` for transmission to the source.

`UNIPD_controlTxDcBusTransferredPowerLimit()` implements Matlab
`Controllo_Tensione_Bus_dc_P_trasf()`:

1. calculate the sign-reversed voltage-squared error because transferred power
   discharges the transmitter bus;
2. generate the transferable-power limit through the discrete PI;
3. clamp it using the configured minimum and maximum coil-current amplitudes.

Both functions have explicit state and reset support. The integration reads the
role published by the existing distributed FSM and uses the two fields already
present in the operational radio payload. The FSM implementation and the nRF24
transport are not modified. PWM outputs remain disconnected.

## Radio field units

The existing signed 16-bit fields use the following scales:

- `powerToLoad`: `1 W/LSB`, range approximately -32.768 kW to +32.767 kW;
- `iCoilErr`: `0.01 A/LSB`, range -327.68 A to +327.67 A.

In SOURCE role the local controller publishes `powerToLoad` and consumes
`iCoilErr`. In LOAD role it consumes `powerToLoad` and publishes `iCoilErr`.
Exchange consistency is an external prerequisite provided by the distributed
state-machine/radio layer.

## UART interface

The computation-only integration is disabled by default. Commands are:

- `WPT?`: diagnostic snapshot;
- `WPT=0`, `WPT=1`: disable/enable UniPD computation only;
- `WPTSRC=<mV>`: SOURCE DCLINK reference;
- `WPTLOAD=<mV>`: LOAD DCLINK reference;
- `WPTIMAX=<mA>`: maximum coil-current amplitude used by the SOURCE power
  limit.
- `WPTISRC=<mA>` / `WPTISRC=OFF`: select or disable the synthetic SOURCE
  coil-current input.
- `WPTILOAD=<mA>` / `WPTILOAD=OFF`: select or disable a synthetic LOAD
  coil-current input. When enabled, it replaces the physical `ITANK_MOD`
  measurement and the `WPTIZERO` correction without changing the UniPD
  controller.

Changing a reference or limit disables the computation and resets its states.
`WPT=1` does not enable or write HFC PWM.

Firmware release 1017 adds the LOAD-only synthetic-current selector and reports
its state and value as `ILSYN` and `ILSYN_mA` in `WPT?`. This diagnostic bypass
is intended to separate control/mapping tests from the unresolved LOAD
`ITANK_MOD` analog-chain behavior.

Firmware release 1018 replaces the fixed state-machine analog transition
thresholds with runtime values. The production defaults remain unchanged
(`VBUS_MIN=50 V`, `IBAT_MIN=1 mA`, `ICOIL_MIN=1 mA`), while the UART commands
`SMVBUS=<V>`, `SMIBAT=<mA>` and `SMICOIL=<mA>` adapt them to a reduced-voltage
test asset. `SMTHR?` reports all three thresholds. These commands configure
transition criteria only; the corresponding test inputs remain `VB=<V>`,
`IB=<mA>` and `IC=<mA>`.

Firmware release 1019 adds `WPTPLIMINIT=<W>` for deterministic reduced-voltage
bench initialization of the SOURCE transferred-power-limit state. The command
disables WPT computation and the HFC actuator, records a pending initial
condition, and requires the normal `WPT=1` re-enable. On the first subsequent
SOURCE calculation the requested value is clamped to the existing physical
limit `Vdc * Icoil_max * 2/pi`; controller coefficients and the following
closed-loop evolution are unchanged. `WPT?` reports the requested value and
pending state as `PSEED_W` and `PSEEDP`.

## Coefficients

The following Matlab `Param_Contr` entries are defined as C macros in
`unipd/unipd_control.c`:

| Matlab entries | C macros | Values |
|---|---|---|
| 12..14 | `UNIPD_RX_PTRASF_A1/B0/B1` | `1`, `0.014046111269511`, `-0.014044685364921` |
| 15..17 | `UNIPD_TX_PTRASF_A1/B0/B1` | `1`, `0.014071403798379`, `-0.014070190824372` |

They were regenerated from the original continuous-time model using its
nominal parameters and Tustin discretization. The reconstruction was checked
against the already imported coil-current controller coefficients (Matlab
entries 7..11).

These remain the original nominal coefficients: 250 V bus, 1.5 mF bus
capacitance, 85 kHz HFC frequency and approximately 47 us control period. They
have not been validated for the 12 V / 83 ohm bench point.

## Remaining activation gates

Before enabling closed-loop distributed HFC power outputs:

- exercise both roles through UART with PWM disabled;
- verify the numerical radio encoding end-to-end;
- explicitly review references and clamps for the low-voltage bench setup;
- review and connect `UNIPD_wptHfcOutput` to the existing HFC phase-shift
  command path.

`WLESS_SM_POWER_CONTROL_ENABLE` and the existing power-output gates are not
changed by this import.

## UART parser table correction

During the first dual-board verification the parser rejected the existing
`SOURCE`, `LOAD`, `AUTO`, `MANUAL`, `STOP` and `INITOK` commands. The parse
table contained 32 entries while its traversal count was hard-coded to 26.
The count is now derived with `sizeof(WLESS_UART_parseTable)`, preventing both
the current omission and future mismatches when commands are added.

## HFC actuator safety wrapper and latched diagnostics

The HFC test actuator connects the SOURCE UniPD phase-shift output to the
existing fixed-safe-PWM primary bridge path. It remains separately gated by
`WPTHFC=0/1`; enabling requires WPT computation, SOURCE/LOAD roles, valid radio
link, SOURCE synthetic-current selection and an allowed SOURCE FSM state.
The primary duty remains fixed at 0.5 and only the inter-leg phase shift is
ramped and clamped. `WPTHFCLIM` and `WPTHFCRAMP` configure the external safety
wrapper without modifying UniPD coefficients.

Firmware release 1005 reduced the default phase limit to 0.005 pu and made
disable explicitly clear the test command and both CLLLC phase references
before forcing the PWM trips.

Firmware release 1006 added latched post-run diagnostics to `HFC?`:

- `F`: exact wrapper stop reason (1 WPT disabled, 2 local role, 3 remote role,
  4 radio link, 5 no-ACK threshold, 6 FSM state, 7 integration dispatcher);
- `LAST_mpu` and `PEAK_mpu`: last and maximum applied phase shift;
- `TICKS`: last calculated ePWM inter-leg phase value;
- `CYC`: number of ISR cycles for which the actuator remained enabled;
- `FVD_mV`: SOURCE DCLINK measurement latched when an interlock stopped it.

The fields reset only on a new accepted `WPTHFC=1`, so they remain available
after an explicit stop or automatic interlock. These additions are diagnostic
and do not alter the UniPD algorithms or distributed FSM implementation.

## HFC inter-leg phase mapping correction

The wireless build keeps the TI secondary bridge disabled. The original PWM
update helper incorrectly placed the primary-leg-2 `TBPHS` write inside the
`CLLLC_SECONDARY_ENABLED` compile-time guard. Consequently, the UniPD phase
reference and calculated ticks changed while the physical EPWM2 phase register
remained zero. The EPWM2 `TBPHS` write and count-direction update now remain
active independently of the removed secondary bridge.

Bench characterization established the retained TI up-down PWM convention:

```text
phase_TI = 0.5 - abs(PSA - 0.5)
```

Thus UniPD neutral (`PSA=PSB=0.5`) maps to `phase_TI=0.5`, while maximum
differential modulation maps to zero. `WPTHFCPH=<0..500>` provides a guarded
manual phase command in milli-pu for actuator characterization;
`WPTHFCPHAUTO` restores the UniPD mapping. The manual command does not bypass
the enable-time role/link checks or local runtime safety checks.

`HFC?` additionally reports the latched EPWM1/EPWM2 `TBPHS`, `TBPRD` and
`TBCTL` registers, manual-mode state, and the remote-role invalid-sample
counter. Automatic UniPD operation retains a 100 ms debounce on an invalid
remote-role field. Manual characterization validates the remote LOAD role at
enable and relies at runtime on link/no-ACK and local role/state interlocks.

## HFC bench interlock update - firmware 1015

For continued closed-loop HFC bench testing, fault reason 3 no longer stops
the actuator when the runtime `remoteRole` field becomes temporarily invalid.
The remote LOAD role is still required by `WPTHFC=1` at enable time. Runtime
interlocks for WPT integration enable, local SOURCE role, radio-link state,
no-ACK threshold, allowed local FSM state and integration dispatcher remain
active. `RRINV` remains available as diagnostic telemetry and does not alter
the distributed state-machine implementation.

The accepted `WPTHFCLIM` range was extended from 1..50 mpu to 1..250 mpu so
that the external HFC actuator clamp can be relaxed progressively during the
bench tests. This changes only the UART-configurable wrapper limit; the UniPD
control algorithm and its coefficients are unchanged.

Firmware 1016 extends the accepted `WPTHFCLIM` range to 1..500 mpu, covering
the complete differential-modulation range supported by the established
mapping. An applied command of 0 mpu is the neutral point
(`phase_TI=0.5`), while 500 mpu maps to maximum differential excitation
(`phase_TI=0`). The conservative default is unchanged; access to the upper
range requires an explicit UART command. UniPD coefficients and the
distributed state machine remain unchanged.

## Atomic WPT diagnostic snapshot - firmware 1020

`WPTSNAP?` captures the WPT integration variables that are updated by the
21.25 kHz ISR in one short interrupt-protected copy. UART formatting and
transmission occur after interrupts have been restored. This prevents a
single diagnostic line from mixing values belonging to different control
cycles, which can happen with the broader, non-atomic `WPT?` status command.

The snapshot reports the FSM tick and step, role, state, radio-link status,
no-ACK count, physical/offset/used coil currents, remote and local transferred
power references, coil-current reference and coil-current error. The command
is diagnostic only and does not change the UniPD controller, its coefficients,
the distributed state machine or an actuator command.

## LOAD transferred-power initial condition - firmware 1021

`WPTPLOADINIT=<W>` provides a bench-test initial condition for the LOAD
transferred-power controller. The command disables the WPT integration and
HFC actuator, stores the requested initial power and marks it pending. After
`WPT=1`, the LOAD consumes the pending value and initializes
`p_trasf_rif_p`, clamped between zero and the power limit currently received
from the SOURCE. The previous squared bus-voltage error is initialized from
the current measured LOAD bus voltage to avoid an artificial one-sample
increment.

`LPSEED_W` and `LPSEEDP` expose the configured value and pending flag in
`WPT?`. This does not change the UniPD equations or controller coefficients;
it changes only their LOAD-side initial state for reduced-voltage bench
operation.

Firmware 1022 keeps the LOAD seed pending while the received SOURCE power
limit is zero. Temporarily disabling the local integration clears the local
copy of `powerToLoad`; therefore consuming the seed on the first re-enabled
cycle would clamp it to zero before the next radio update. The seed is now
consumed only after a positive remote limit is available.

Firmware 1023 removes the redundant global `UNIPD_resetAllControls` request
from `WPTPLOADINIT`. Disabling WPT integration already clears the transferred
power states directly. On the STATION, the global reset can be delayed by the
BBC docking-test dispatcher and execute after the pending seed has been
consumed, erasing the initialized LOAD state.

## LOAD seed synchronized with physical plant startup - firmware 1053

Bench diagnostics showed that `WPT=1` starts the transferred-power equations
before `WPTHFC=1` enables the physical HFC actuator. The SOURCE power limit can
therefore already be positive while the measured LOAD DCLINK is still zero.
In that condition the LOAD consumed `WPTPLOADINIT`, and its controller state
continued evolving before the plant was active.

Firmware 1053 keeps the LOAD seed pending until both the remote SOURCE power
limit is positive and the physical LOAD DCLINK measurement reaches
`UNIPD_vDcMinAlg_Volts`. The first observable transfer therefore initializes
the LOAD power state. UniPD equations and coefficients, the radio payload and
the HFC actuator mapping are unchanged.

## Separate local and remote transferred-power values - firmware 1054

Closed-loop captures showed that the LOAD received a positive SOURCE power
limit while its local `PREF` remained close to zero. Static analysis confirmed
that the radio decoder stored the received value in `WLESS_SM_powerToLoad`, the
same object used for the local transmitted command and periodically cleared by
the LOAD state machine.

Firmware 1054 introduces `WLESS_SM_remotePowerToLoad`. The radio receiver writes
the remote snapshot there, while UniPD LOAD reads it as its received power
limit. `WLESS_SM_powerToLoad` remains exclusively the locally generated value
serialized into the payload. The payload layout, controller coefficients and
state-machine transitions are unchanged.

## Extended LOAD power-loop capture - firmware 1055

The firmware 1054 bench run confirmed that separating the remote power value
removed a real local/remote data collision, but cyclic LOAD-voltage dropouts
remained. The existing capture rounded `PREF` to integer watts and did not
record the received power limit or the controller state used by the difference
equation.

Firmware 1055 changes diagnostics only. `WPTCAPD` schema 2 adds the received
limit and raw payload value, `PREF` and its previous value in milliwatts, the
current and previous squared-voltage errors, LOAD-seed state and pending reset
command. No controller coefficient, FSM transition, radio payload or PWM
command is changed.

The initial implementation enlarged every capture sample by 11 C28x words and
made the 96-sample `wptCapture` section exceed RAMGS3 (`0x1140` words requested
versus `0x0ff8` available). The final implementation keeps the original sample
size (`0x0d20` words for the complete buffer) by reusing the HFC-actuator slots
on LOAD, where that actuator is not operational. Consequently, in schema 2 the
`PREM` through `RESET` fields are meaningful on LOAD, whereas the `VACRAW`
through `HPHY` aliases are meaningful on SOURCE. This is a diagnostic storage
overlay only; it does not alter controller or actuator data.

## Automatic LOAD power-reference drop capture

`WPTCAP=2` arms the existing ring buffer with an automatic LOAD-side trigger.
The buffer freezes when `PREF` falls by at least 100 mW between consecutive
captured samples, provided the previous value was at least 100 mW. With
`WPTCAPDEC=21` the sampling rate is approximately 1 kHz and the 96-sample
buffer retains about 95 ms of pre-event history plus the triggering sample.

The LOAD overlay field formerly named `RESET` is now `CLEARSEQ`, a monotonic
counter of calls to `unipd_clearTransferredPowerIntegration()` reset whenever
the capture is armed. Unlike the command flag, it cannot be asserted and
consumed between samples without leaving evidence.

`WPTCAP=1` retains the previous manual capture behavior. `WPTCAP=0` still
freezes the buffer manually.

The initial 1056 bench preparation showed that a positive remote power limit
can make `PREF` evolve while the physical LOAD DCLINK is still zero, causing a
false automatic trigger before HFC startup. Firmware 1057 therefore clears the
trigger history and inhibits drop detection while measured LOAD `VDC` is below
`UNIPD_vDcMinAlg_Volts`. Controller equations and outputs are unchanged.

The first active-plant capture with firmware 1057 then showed a second false
trigger: the normal LOAD startup caused `PREF` to fall as the squared-voltage
error decreased. Firmware 1058 qualifies drop detection only after measured
LOAD `VDC` reaches 80% of `UNIPD_wptVdcLoadRef_Volts`. Once qualified, the
trigger remains enabled even if `VDC` subsequently falls, so an actual dropout
can still freeze the buffer. Controller equations and outputs remain unchanged.

The active-plant capture with firmware 1058 showed that a 100 mW `PREF` drop
threshold could still trigger on the normal settling immediately after the 80%
qualification point. Firmware 1059 therefore uses the physical LOAD voltage as
the trigger: the capture is qualified at 80% of the LOAD reference and freezes
when the voltage subsequently falls below 70%. `REASON=2` identifies this
physical dropout trigger. The buffer still records `PREF`, previous controller
state and `CLEARSEQ`, but those values no longer decide when it freezes.

Firmware 1060 adds the corresponding SOURCE-side diagnostic trigger. On a
SOURCE capture, the `PREF` storage field records the local integer
`WLESS_SM_powerToLoad` that is published in the operational payload. The trigger
is qualified after that local value reaches at least 10 W and freezes if it
subsequently becomes zero (`REASON=3`). Running `WPTCAP=2` on both roles can
therefore distinguish a zero generated locally by the SOURCE from a zero first
observed by the LOAD after payload composition, transport or consumption. The
LOAD voltage trigger remains `REASON=2`.

Firmware 1061 moves the SOURCE discriminator to the exact payload boundary.
`WLESS_NRF24_makeOperationalPayload()` records the value copied into TX bytes
4/5 and latches a transition to zero after a value of at least 10 W. The
corresponding value decoded by `WLESS_NRF24_consumeOperationalPayload()` has an
independent RX latch. `WPTCAP=2` clears these trace latches before arming;
`RADIO?` exposes `TXP`, `RXP`, `TXP0` and `RXP0`. The SOURCE capture now freezes
from the TX-builder latch (`REASON=3`), eliminating the sampling gap present in
firmware 1060. The operational payload format and radio timing are unchanged.
