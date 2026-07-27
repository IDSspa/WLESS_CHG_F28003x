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
- `C223`, `C224`, and `C225` are 470 uF polarized bulk capacitors on `DCLINK` to `PGND`.
- `R159`, `R164`, `R166`, `R171`, and `R172` are 10 mOhm shunts in parallel, equivalent to 2 mOhm.
- Rev. 1.2 of the project DT identifies the coil interface as two series
  capacitor banks plus the coil-current sensor. Some capacitors in the DT
  schematic are marked `DNM`, so the physically mounted capacitance must be
  verified before high-power resonance tests.

## Power Nets

| Electrical net | Physical meaning | Firmware/current software name | Notes |
|---|---|---|---|
| `DCLINK` | Positive DC bus between BBC and HFC | `TTPLPFC_vBus_sensed_pu`, UniPD `v_dc` | `J38`, positive side of C223/C224/C225 |
| `PGND` | Power ground / battery negative return | none | `J37`, negative side of C223/C224/C225 |
| `IBUS_SENSOR` / `IPRI_SENSE` | BBC low-side source return before shunt network | legacy `CLLLC_iPrimSensed_*` area | Name is legacy from original TI primary-side design; Rev. 1.2 names this net `IBUS_SENSOR` |
| `TP5` | Test point on/near `IPRI_SENSE` | none | Returns to `PGND` through the 2 mOhm equivalent shunt |
| `COIL` | Virtual external coil connection | UniPD `i_coil_loc` indirectly via sensed signal | Physical connection exists as wiring point, not connector |

## Measurement Scaling From Project DT Rev. 1.2

These formulas are taken from the updated project DT and should be used as the
starting point for firmware calibration. Bench calibration remains mandatory on
the modified board.

| Quantity | DT Rev. 1.2 relation | Practical note |
|---|---|---|
| Battery/input voltage | `VIN_ADC ~= 0.02 * VIN` | Measurement range up to about 165 Vdc |
| DC bus voltage | `VBUS ~= 0.0126 * DCLINK` | Firmware bench correction already made so `TTPLPFC_vBus_sensed_Volts` matches multimeter DCLINK |
| Battery/bus current | `IBUS ~= 1.65 V + 0.03 V/A * I` | Based on low-side return current through the shunt/sense chain |
| BBC phase current | `IDC_PHx ~= 1.65 V + 0.04125 V/A * I_PHx` | Bidirectional branch currents: 1.65 V = 0 A, 3.3 V = +40 A, 0 V = -40 A |
| Coil current sensor | `ITANK_SENSOR ~= 1.65 V + 0.020 V/A * Icoil` | Sensor output before conditioning |
| Coil current conditioned | `ITANK ~= 1.65 V + 0.025 V/A * Icoil` | ADC-facing signal after gain stage |
| Coil current magnitude | `ITANK_MOD ~= (6/pi) * 0.025 V/A * |Icoil|` | Conditioned signal routed to J26.31 / ADCC11; 1 V ~= 21 A, ADC full-scale ~= 69.1 A |
| Coil phase | `ITANK_PHS` | Not populated/valid on current modified boards; J26.28 / ADCA5 samples noise only because the resonance-frequency correction circuit is not yet included in this prototype |

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

Project DT Rev. 1.2 confirms the same logical grouping:

- `PH1_H` / `PH1_L`: PWM6 A/B, first BBC branch.
- `PH2_H` / `PH2_L`: PWM7 A/B, second BBC branch.
- `SW1` and `SW2`: switching nodes between high-side source and low-side drain.

Bench status on docking:

- `EPWM6A/B` and `EPWM7A/B` generate 120 kHz docking-test signals when
  `TTPLPFC_bbcDockTestEnable = 1`.
- Duty variables `TTPLPFC_bbcDockTestDuty1_pu` and
  `TTPLPFC_bbcDockTestDuty2_pu` affect the corresponding EPWM outputs.
- `TTPLPFC_bbcDockTestEnable = 0` disables the BBC PWM outputs.

## ADC/Measurement Mapping

| Electrical quantity | Firmware raw/per-unit variable | ADC channel | UniPD input | Bench status |
|---|---|---|---|---|
| DC bus voltage | `TTPLPFC_vBus_sensed_pu` | `ADCB4` | `v_dc` | Tested on docking and modified board, J26 pin 20 |
| BBC inductor/current A | `TTPLPFC_iL1_sensed_pu` | `ADCB12` after current firmware swap | `i_l_a` | J26 pin 15 / `IDC_PH1`; sign/offset still to be calibrated on real board |
| BBC inductor/current B | `TTPLPFC_iL2_sensed_pu` | `ADCA2` after current firmware swap | `i_l_b` | J26 pin 18 / `IDC_PH2`; sign/offset still to be calibrated on real board |
| Coil current magnitude | `CLLLC_iTankModSensed_pu` | `ADCC11` | `i_coil_loc` | Modified-board mapping: `ITANK_MOD`, J26 pin 31 |
| Unused/noise diagnostic | `CLLLC_unusedAdca5Sensed_pu` | `ADCA5` | not used | J26 pin 28 is not driven by a valid `ITANK_PHS` circuit |
| Battery/bus current | TBD | TBD | not currently used by UniPD wrapper | DT Rev. 1.2: J26 pin 34 / `IBUS` |
| Direct coil current | TBD | TBD | not currently used by UniPD wrapper | DT Rev. 1.2: J26 pin 37 / `ITANK` |
| Battery/input voltage | `TTPLPFC_vBatSensed_Volts` | `ADCC7`, SOC13 | `v_bat` | DT Rev. 1.2: J26 pin 39 / `VIN_ADC`; controlCARD Rev.B schematic maps J26.39 to ADCA3/ADCB9/ADCC7; firmware selects ADCC7; nominal conversion pending bench calibration |

Current software offsets:

- `CLLLC_iTankModSensedOffset_pu`
- `CLLLC_unusedAdca5SensedOffset_pu` diagnostic only, not a control offset

Both default to zero and can be adjusted manually during bench calibration.

## ControlCARD Boundary

The reverse schematic should keep the controlCARD as a boundary, not duplicate
the MCU internals. Recommended table columns for the KiCad sheet:

| Board net | J26/controlCARD pin | MCU signal | Firmware name | Direction | Test note |
|---|---:|---|---|---|---|
| `LEG1_H` | 49 | `GPIO0 / EPWM1A` | `CLLLC_PRIM_LEG1_H` | output | HFC PWM verified |
| `LEG1_L` | 51 | `GPIO1 / EPWM1B` | `CLLLC_PRIM_LEG1_L` | output | HFC PWM verified |
| `LEG2_H` | 53 | `GPIO2 / EPWM2A` | `CLLLC_PRIM_LEG2_H` | output | HFC PWM verified |
| `LEG2_L` | 55 | `GPIO3 / EPWM2B` | `CLLLC_PRIM_LEG2_L` | output | HFC PWM verified |
| `PWM1_A90DEG` | 50 | `GPIO4 / EPWM3A` | `CLLLC_PWM3_SYNC90` | output | 90 deg delay verified |
| `PWM1_B90DEG` | 52 | `GPIO5 / EPWM3B` | unused/disabled for WLESS | output | Keep disabled unless needed |
| `HFC_FAULTn` / aggregated GaN fault | 74 | `GPIO23 / XBAR_INPUT2 / TZ2` | `CLLLC_GANFAULTn_GPIO` | input | Hardware OR of `CLLLC_PRIM_FAULT_*` lines, observed as `ganFaultTrip` / OST2 |
| `CLLLC_GAN_OC_PRIM_LEG1_H` | 99 | TBD | not currently used | input | Individual HFC GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_PRIM_LEG2_H` | 109 | TBD | not currently used | input | Individual HFC GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_PRIM_LEG1_L` | 107 | TBD | not currently used | input | Individual HFC GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_PRIM_LEG2_L` | 100 | TBD | not currently used | input | Individual HFC GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_SEC_LEG1_H` | 104 | TBD | not currently used | input | Individual removed-secondary GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_SEC_LEG2_H` | 106 | TBD | not currently used | input | Individual removed-secondary GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_SEC_LEG1_L` | 108 | TBD | not currently used | input | Individual removed-secondary GaN overcurrent diagnostic |
| `CLLLC_GAN_OC_SEC_LEG2_L` | 110 | TBD | not currently used | input | Individual removed-secondary GaN overcurrent diagnostic |
| `DUMMY_PWMA` | 57 | `GPIO8 / EPWM5A` | legacy dummy PWM | output | Disabled in WLESS tests |
| `DUMMY_PWMB` | 59 | `GPIO9 / EPWM5B` | legacy dummy PWM | output | Disabled in WLESS tests |
| `PH1_H` | 61 | `GPIO10 / EPWM6A` | `TTPLPFC_BBC_LEG1_HIGH_GPIO` | output | BBC docking and board test verified |
| `PH1_L` | 63 | `GPIO11 / EPWM6B` | `TTPLPFC_BBC_LEG1_LOW_GPIO` | output | BBC docking and board test verified |
| `PH2_H` | 58 | `GPIO12 / EPWM7A` | `TTPLPFC_BBC_LEG2_HIGH_GPIO` | output | BBC docking and board test verified |
| `PH2_L` | 60 | `GPIO13 / EPWM7B` | `TTPLPFC_BBC_LEG2_LOW_GPIO` | output | BBC docking and board test verified |
| `IDC_PH1` | 15 | TBD | `TTPLPFC_iL1_sensed_pu` / UniPD `i_l_a` | input | Docking ADC injection verified |
| `IDC_PH2` | 18 | TBD | `TTPLPFC_iL2_sensed_pu` / UniPD `i_l_b` | input | Docking ADC injection verified |
| `VBUS` | 20 | `ADCB4` | `TTPLPFC_vBus_sensed_pu` | input | Docking and board measurement verified |
| unused / `ITANK_PHS` placeholder | 28 | `ADCA5` | `CLLLC_unusedAdca5Sensed_pu` | input | No valid phase-conditioning circuit on current boards; omitted until resonance-frequency correction is implemented |
| `ITANK_MOD` | 31 | `ADCC11` | `CLLLC_iTankModSensed_pu` / UniPD `i_coil_loc` | input | Modified-board schematic mapping confirmed |
| `IBUS` | 34 | TBD | candidate battery/bus current input | input | DT Rev. 1.2 confirmed, firmware mapping TBD |
| `ITANK` | 37 | TBD | candidate direct coil-current input | input | DT Rev. 1.2 confirmed, firmware mapping TBD |
| `VIN_ADC` | 39 | `ADCA3/ADCB9/ADCC7`; firmware: `ADCC7` | `TTPLPFC_vBatAdcRaw`, `TTPLPFC_vBatSensed_Volts`, UniPD `v_bat` | input | DT Rev. 1.2 and controlCARD Rev.B schematic confirmed; nominal `VIN_ADC ~= 0.02 * VBATT`, bench calibration pending |

HFC fault aggregation noted from the original schematic: the active-low
`CLLLC_PRIM_FAULT_LEG1_H`, `CLLLC_PRIM_FAULT_LEG2_H`,
`CLLLC_PRIM_FAULT_LEG1_L`, and `CLLLC_PRIM_FAULT_LEG2_L` lines are
OR-combined by U31 and then by U30. The final aggregated active-low output
reaches J26.74 / GPIO23 and drives the firmware GaN fault path through
`XBAR_INPUT2` and ePWM one-shot trip `OST2`. The four individual
`CLLLC_GAN_OC_PRIM_*` diagnostic lines are routed separately to J26 pins 99,
109, 107, and 100. The removed-secondary `CLLLC_GAN_OC_SEC_*` diagnostic
lines are routed separately to J26 pins 104, 106, 108, and 110. Firmware
currently only uses the aggregated fault line.

## Open Items

- Verify physically mounted coil-compensation capacitors. DT Rev. 1.2 shows
  DNM options and states a target resonant capacitance of either 82.5 nF for
  the 45 uH design point or 66 nF for the 55-57 uH coupled-coil cases.
- Map firmware ADC channels for `IBUS` and `ITANK` if those signals
  are required by the closed-loop controls.
- Calibrate `ITANK_MOD` offset/scale on the modified boards before real-board
  closed-loop control. Do not use ADCA5 / `ITANK_PHS` for rectifier control on
  the current hardware.
- Calibrate sign and offset of real-board current sensors before closed-loop
  tests.
