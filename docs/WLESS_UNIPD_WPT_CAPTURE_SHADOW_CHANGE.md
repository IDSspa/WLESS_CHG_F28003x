# WLESS UniPD WPT synchronized capture and shadow diagnostics

## Scope

This document describes the functional diagnostic additions introduced in
firmware release 1024. It does not report bench-test results.

The purpose is to:

1. capture WPT control variables from one ISR execution context without
   transmitting UART data during the control interval;
2. retain UniPD computation while a manual HFC phase is physically applied;
3. compare the automatic phase that UniPD would apply with the manually
   imposed physical phase.

No UniPD coefficients, equations, clamps, state-machine transitions or nRF
payload formats are changed.

## Local synchronized WPT capture

Each controlCARD owns an independent circular buffer of 96 samples. A sample
is copied at the end of `UNIPD_runTransferredPowerIntegration()`, which runs
in ISR2. Values in one sample therefore belong to one local ISR execution.

Captured fields are:

```text
tick
local role, state and radio-link state
local Vdc/VLOAD
physical and selected local coil-current values
PREF, IREF and IERR
limited VAC reference
HREQ and HAPP
automatic hardware phase (HAUTO)
physical hardware phase (HPHY)
manual-mode flag
HFC fault
```

`PREF/IREF/IERR` are meaningful on the LOAD controller. `VAC/HREQ/HAPP` and
the HFC phase fields are meaningful on the SOURCE controller. The two local
buffers are not globally time-synchronized; radio-derived values are the
most recently received values available in the local ISR.

The buffer is placed in the dedicated linker section `wptCapture` in RAMGS3,
avoiding the nearly full ordinary `.bss` region in RAMGS2.

## Decimation and time window

The default decimation is 21 ISR2 executions. With the nominal ISR2
frequency of 21.25 kHz this is approximately one sample per millisecond and
approximately 96 ms for a complete buffer.

The decimation can be changed before arming:

```text
WPTCAPDEC=<1..65535>
```

Examples:

```text
WPTCAPDEC=21     approximately 1 ms/sample, 96 ms window
WPTCAPDEC=213    approximately 10 ms/sample, 0.96 s window
WPTCAPDEC=2125   approximately 100 ms/sample, 9.6 s window
```

Changing decimation while the buffer is armed is rejected.

## UART commands

```text
WPTCAP=1       clears and arms the circular buffer
WPTCAP=0       stops and freezes the buffer
WPTCAP?        reports ARM, FROZEN, COUNT, LEN and DEC
WPTCAPD?       dumps the frozen buffer in chronological CSV-like rows
WPTCAPDEC=n    sets the ISR decimation before arming
```

Dumping while armed is rejected so UART formatting cannot race the ISR
writer. Once full, the buffer remains circular and retains the most recent
96 samples until stopped.

## Shadow-mode semantics

The existing manual command remains:

```text
WPTHFCPH=<0..500>
```

UniPD continues to calculate the automatic differential request and its
ramped value. Manual mode changes only the phase sent to the HFC hardware.
Release 1024 records four distinct quantities:

```text
HREQ   UniPD differential phase request
HAPP   ramped UniPD differential phase
HAUTO  physical TI phase that would be applied automatically: 0.5 - HAPP
HPHY   physical TI phase actually sent to the HFC driver
```

In automatic mode `HAUTO` and `HPHY` are equal. In manual mode `HPHY`
follows `WPTHFCPH`, while `HAUTO` continues to expose the control decision
that UniPD would have applied.

## Resource use and build verification

The WPT buffer occupies 0x0B40 16-bit words in RAMGS3. The linker reports
0x04B8 words free in that region after the VEHICLE build.

Full-clean builds completed successfully for both roles:

```text
RELEASE/WLESS_CHG_F28003x_VEHICLE_FW1024.out
RELEASE/WLESS_CHG_F28003x_STATION_FW1024.out
```

The source tree is left with `WLESS_SM_BUILD_VEHICLE=1`.
