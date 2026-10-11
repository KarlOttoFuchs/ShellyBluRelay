# CLAUDE.md

The ESP32-C3 Relay Module, `FEHA-RM-001` Rev A: fabricated, and frozen. Public repository.

- `Hardware/ESP32C3-Relay-Module-Rev-A/` — KiCad files as fabricated; they must match the
  boards in hand, so they are not edited. Symbols and footprints resolve from machine-global
  libraries.
- `ESPHome/` — the current firmware: ESPHome configuration with Shelly BLU (BTHome v2) support.
- `Firmware/esp32c3-relay-firmware/` — the original ESP-IDF firmware, superseded by `ESPHome/`.
- `_bmad-output/` — planning artifacts of the original ESP-IDF firmware.

Working agreement: Karl is an experienced hardware engineer — keep chat short. Ask when unsure;
propose commits and wait for his go-ahead. Never commit `ESPHome/secrets.yaml`.
