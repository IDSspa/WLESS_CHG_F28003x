# Wireless Charger Net/Firmware Mapping

This document maps the reverse-engineered power-board nets to the firmware names
used in the TI codebase. It is intended as a working reference for bench tests on
the modified TI board.

## Schematic Scope

Reverse schematic source:

- `C:\Users\m.santucci\OneDrive - fincantieri.it\Progetti\Wireless Charger\SchedaTIModificata\SchedaTIModificata.kicad_sch`
- `C:\Users\m.santucci\OneDrive - fincantieri.it\Progetti\Wireless Charger\SchedaTIModificata\control_card.kicad_sch`

Known schematic notes:

- `J38` is `DCLINK`.
- `J37` is `PGND`.
- `J39` is a virtual `COIL` connection point, not a physical connector.
- `C1` and `C2` are schematic placeholders, not confirmed physical components.
- `C223`, `C224`, and `C225` are 470 uF polarized bulk capacitors on `DCLINK` to `PGND`.
- `R159`, `R164`, `R166`, `R171`, and `R172` are 10 mOhm shunts in parallel, equivalent to 2 mOhm.

## Power Nets

| Electrical net | Physical meaning | Firmware/current software name | Notes |
|---|---|---|---|
| `DCLINK` | Positive DC bus between BBC and HFC | `TTPLPFC_vBus_sensed_pu`, UniPD `v_dc` | `J38`, positive side of C223/C224/C225 |
| `PGND` | Power ground / battery negative return | none | `J37`, negative side of C223/C224/C225 |
| `IPRI_SENSE` | BBC low-side source return before shunt network | legacy `CLLLC_iPrimSensed_*` area | Name is legacy from original TI primary-side design |
| `TP5` | Test point on/near `IPRI_SENSE` | none | Returns to `PGND` through the 2 mOhm equivalent shunt |
| `COIL` | Virtual external coil connection | UniPD `i_coil_loc` indirectly via sensed signal | Physical connection exists as wiring point, not connector |

## HFC PWM Nets

The HFC bridge reuses the original CLLLC primary bridge hardware. Firmware
currently keeps legacy `CLLLC_PRIM_*` names.

| Electrical net | Firmware PWM base/GPIO | Observed/test status | Notes |
|---|---|---|---|
| `CLLLC_PRIM_LEG1_H` | `EPWM1A`, `GPIO0` | present, 85 kHz | Main HFC leg 1 high-side command |
| `CLLLC_PRIM_LEG1_L` | `EPWM1B`, `GPIO1` | present, complementary to leg 1 high-side | Main HFC leg 1 low-side command |
| `CLLLC_PRIM_LEG2_H` | `EPWM2A`, `GPIO2` | present, 85 kHz | Main HFC leg 2 high-side command |
| `CLLLC_PRIM_LEG2_L` | `EPWM2B`, `GPIO3` | present, complementary to leg 2 high-side | Main HFC leg 2 low-side command |
| wireless sync/test PWM | `EPWM3A`, `GPIO4` | present, 90 deg delayed from `EPWM1A` | Reuses former secondary PWM channel as wireless timing signal |

## BBC PWM Nets

The BBC bridge reuses the original high-frequency PFC devices. Current firmware
selects BBC ownership of `EPWM6` and `EPWM7` through
`TTPLPFC_EPWM67_ACTIVE_CONTROL = TTPLPFC_EPWM67_CONTROL_BBC`.

Assumed mapping, pending final hardware confirmation from the project technical
lead:

| Electrical net | Original TI/PFC leg | Firmware PWM base/GPIO | Proposed BBC role | Notes |
|---|---|---|---|---|
| `TTPLPFC_HIGH_FREQ_PH1_H` | PFC phase 1 high-side | `EPWM6A`, `GPIO10` | BBC leg 3 high-side | Drives U37 through original isolator path |
| `TTPLPFC_HIGH_FREQ_PH1_L` | PFC phase 1 low-side | `EPWM6B`, `GPIO11` | BBC leg 3 low-side | Drives U35 through original isolator path |
| `TTPLPFC_HIGH_FREQ_PH2_H` | PFC phase 2 high-side | `EPWM7A`, `GPIO12` | BBC leg 4 high-side | Drives U41 through original isolator path |
| `TTPLPFC_HIGH_FREQ_PH2_L` | PFC phase 2 low-side | `EPWM7B`, `GPIO13` | BBC leg 4 low-side | Drives U39 through original isolator path |

Bench status on docking:

- `EPWM6A/B` and `EPWM7A/B` generate 120 kHz docking-test signals when
  `TTPLPFC_bbcDockTestEnable = 1`.
- Duty variables `TTPLPFC_bbcDockTestDuty1_pu` and
  `TTPLPFC_bbcDockTestDuty2_pu` affect the corresponding EPWM outputs.
- `TTPLPFC_bbcDockTestEnable = 0` disables the BBC PWM outputs.

## ADC/Measurement Mapping

| Electrical quantity | Firmware raw/per-unit variable | ADC channel | UniPD input | Bench status |
|---|---|---|---|---|
| DC bus voltage | `TTPLPFC_vBus_sensed_pu` | `ADCB4` | `v_dc` | Tested on docking, user wiring used controlCARD pin 20 |
| BBC inductor/current A | `TTPLPFC_iL1_sensed_pu` | `ADCB12` after current firmware swap | `i_l_a` | Tested on docking; sign/offset still to be calibrated on real board |
| BBC inductor/current B | `TTPLPFC_iL2_sensed_pu` | `ADCA2` after current firmware swap | `i_l_b` | Tested on docking; sign/offset still to be calibrated on real board |
| Coil current magnitude | `CLLLC_iPrimTankModSensed_pu` | `ADCA5` | `i_coil_loc` | Tested on docking, controlCARD pin 21 |
| Coil current phase | `CLLLC_iPrimTankPhsSensed_pu` | `ADCC11` | not currently used by UniPD wrapper | Tested on docking, controlCARD pin 31 |

Current software offsets:

- `CLLLC_iPrimTankModSensedOffset_pu`
- `CLLLC_iPrimTankPhsSensedOffset_pu`

Both default to zero and can be adjusted manually during bench calibration.

## ControlCARD Boundary

The reverse schematic should keep the controlCARD as a boundary, not duplicate
the MCU internals. Recommended table columns for the KiCad sheet:

| Board net | J26/controlCARD pin | MCU signal | Firmware name | Direction | Test note |
|---|---:|---|---|---|---|
| `CLLLC_PRIM_LEG1_H` | TBD | `GPIO0 / EPWM1A` | `CLLLC_PRIM_LEG1_H` | output | HFC PWM verified |
| `CLLLC_PRIM_LEG1_L` | TBD | `GPIO1 / EPWM1B` | `CLLLC_PRIM_LEG1_L` | output | HFC PWM verified |
| `CLLLC_PRIM_LEG2_H` | TBD | `GPIO2 / EPWM2A` | `CLLLC_PRIM_LEG2_H` | output | HFC PWM verified |
| `CLLLC_PRIM_LEG2_L` | TBD | `GPIO3 / EPWM2B` | `CLLLC_PRIM_LEG2_L` | output | HFC PWM verified |
| wireless sync/test PWM | TBD | `GPIO4 / EPWM3A` | `CLLLC_PWM3_SYNC90` | output | 90 deg delay verified |
| `TTPLPFC_HIGH_FREQ_PH1_H` | TBD | `GPIO10 / EPWM6A` | `TTPLPFC_BBC_LEG1_HIGH_GPIO` | output | BBC docking test verified |
| `TTPLPFC_HIGH_FREQ_PH1_L` | TBD | `GPIO11 / EPWM6B` | `TTPLPFC_BBC_LEG1_LOW_GPIO` | output | BBC docking test verified |
| `TTPLPFC_HIGH_FREQ_PH2_H` | TBD | `GPIO12 / EPWM7A` | `TTPLPFC_BBC_LEG2_HIGH_GPIO` | output | BBC docking test verified |
| `TTPLPFC_HIGH_FREQ_PH2_L` | TBD | `GPIO13 / EPWM7B` | `TTPLPFC_BBC_LEG2_LOW_GPIO` | output | BBC docking test verified |
| `DCLINK` sense | TBD | `ADCB4` | `TTPLPFC_vBus_sensed_pu` | input | Docking ADC injection verified |
| current A sense | TBD | `ADCB12` | `TTPLPFC_iL1_sensed_pu` / UniPD `i_l_a` | input | Docking ADC injection verified |
| current B sense | TBD | `ADCA2` | `TTPLPFC_iL2_sensed_pu` / UniPD `i_l_b` | input | Docking ADC injection verified |
| coil magnitude sense | TBD | `ADCA5` | `CLLLC_iPrimTankModSensed_pu` / UniPD `i_coil_loc` | input | Docking ADC injection verified |
| coil phase sense | TBD | `ADCC11` | `CLLLC_iPrimTankPhsSensed_pu` | input | Docking ADC injection verified |

## Open Items

- Confirm final BBC leg mapping with the project technical lead:
  - old PFC phase 1 -> BBC leg 3
  - old PFC phase 2 -> BBC leg 4
- Confirm whether any real resonant/compensation network exists outside the
  reverse-engineered board.
- Fill `J26/controlCARD pin` numbers from the updated KiCad/controlCARD sheet.
- Calibrate sign and offset of real-board current sensors before closed-loop
  tests.
