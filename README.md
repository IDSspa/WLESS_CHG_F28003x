Firmware running on WLESS_CHRG controller boards.
---

FIRMWARE BUILD AND FLASH INSTRUCTIONS

Use build_station.ps1 to build STATION RELEASE firmware.

Use vehicle_station.ps1 to build VEHICLE RELEASE firmware.

Use flash_*.ps1 script to properly flash controlCARD.

ATTENTION only one controlCARD can be attached to programming PC during flashing.

---

TODO:
1) review SM code and atomize shared data collect/dispatch
2) review UART message handling using a more flexible implementation (message handling function pointers)
3) code restart function trough WD feature and map as UART command (test only feature, protected)
4) code calibration and persistent configuration support (flash memory)