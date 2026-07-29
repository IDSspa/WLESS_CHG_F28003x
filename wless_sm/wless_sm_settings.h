#ifndef WLESS_SM_SETTINGS_H_
#define WLESS_SM_SETTINGS_H_

//
// Wireless charger state-machine integration.
//
// Keep the module disabled by default while hardware bring-up is in progress.
// Enabling this only runs the supervisory state machine; power-stage commands
// remain blocked unless WLESS_SM_POWER_CONTROL_ENABLE is explicitly enabled.
//
#define WLESS_SM_ENABLE                 1
#define WLESS_SM_POWER_CONTROL_ENABLE   0

//
// Diagnostic bring-up isolation. When enabled, run only WLESS_SM_init() and
// keep the ISR2 tick hook and A1 state-machine background processing disabled.
//
#define WLESS_SM_TEST_INIT_ONLY         0
#define WLESS_SM_TEST_ISR_TICK_ONLY     0
#define WLESS_SM_TEST_TICK_SERVICE_ONLY 0
#define WLESS_SM_TEST_RUN_DISPATCH_ONLY 0
#define WLESS_SM_TEST_VEHICLE_STANDBY_ONLY 0
#define WLESS_SM_TEST_VEHICLE_OPERATION_STATES_ONLY 0

//
// Build role. The original UniPD communication project uses VEHICLE as a
// compile-time selector. Build scripts override this default from the compiler
// command line so selecting a role never modifies a tracked source file.
//
#ifndef WLESS_SM_BUILD_VEHICLE
#define WLESS_SM_BUILD_VEHICLE          0
#endif

//
// The original nRF demo uses a 1 kHz timing base. Keep Timer1 free for the TI
// firmware and derive the state-machine timing from the existing ISR2 source.
// The state-machine code still decimates diagnostic traffic to ~1 Hz when no
// energy transfer is active.
//
#define WLESS_SM_SOURCE_TICK_HZ         21250U
#define WLESS_SM_BASE_TICK_HZ           1000U
#define WLESS_SM_DIAGNOSTIC_TICKS       WLESS_SM_BASE_TICK_HZ
#define WLESS_SM_OPERATION_TICKS        1U
#define WLESS_SM_PENDING_TICK_LIMIT     8U

#define WLESS_SM_CAPACITY_MAX_WH        4800L
#define WLESS_SM_CAPACITY_MIN_WH        960L
#define WLESS_SM_CAPACITY_THRESHOLD_WH  2880L

#define WLESS_SM_VBUS_MIN_V             50U
#define WLESS_SM_IBAT_MIN_MA            1U
#define WLESS_SM_ICOIL_MIN_MA           1U

// Initial engineering values; keep configurable until validated against the
// final sensors and power-stage dynamics.
#define WLESS_SM_ENERGY_HYSTERESIS_WH   50L
#define WLESS_SM_ROLE_DEADBAND_WH        50L
#define WLESS_SM_ANALOG_CONFIRM_SAMPLES  2U

// Oscilloscope-only scheduler test. Toggle GPIO30 on every generated logical
// tick. Disable again after the quantitative docking measurement.
#define WLESS_SM_TICK_GPIO_TEST_ENABLE    0
#define WLESS_SM_TICK_GPIO_PIN            30U

#endif
