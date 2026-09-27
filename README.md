# ATU-1k-firmware

STM32F405RGT6-based Antenna Tuning Unit controller with CAT/TCI-facing architecture, relay matrix control, and ILI9341 display integration points.

## Repository status

This repository now contains production-oriented application modules and interfaces for the ATU control layer, while keeping hardware bring-up behind clear HAL-facing APIs. It does **not** include CubeMX-generated peripheral initialization or claim a hardware-validated firmware build.

## Implemented core modules

- `App/Src/cat_parser.c` - streaming Yaesu, Kenwood, and Icom CI-V frequency parsing
- `App/Src/frequency_manager.c` - CAT/counter/manual source arbitration and preset lookup flow
- `App/Src/tuner.c` - non-blocking full-tune / fast-tune state machine
- `Drivers/ADC/Src/power_meter.c` - ADC batch processing for forward/reverse/net power and SWR
- `App/Src/debug_log.c` - bounded `#DEBUG:` USB CDC text logging
- `App/Src/rtc_backup.c` - backup-domain snapshot packing/validation
- `App/Src/menu.c` - 10-item event-driven settings menu
- `App/Src/button.c` - debouncing, repeat, and TUNE long-press handling
- `App/Src/storage.c` - FRAM settings/preset layout helpers for two preset banks

## Integration notes

- Frequency source priority follows the documented hardware rule set:
  - USB present + fresh CAT data -> CAT frequency
  - USB present + stale CAT -> manual mode (PD2 counter is blocked externally)
  - USB absent -> frequency counter
- CAT timeout is 5 seconds.
- Presets are applied automatically when a new valid supported frequency maps to a stored entry in the active FRAM bank.
- Successful tuning does **not** auto-save. Long TUNE press is the explicit save action.
- Power metering assumes a linear detector where ADC full scale corresponds to 1600 W into 50 ohms.

## Build / validation

- No target build system was present in the repository, so the new modules are kept self-contained and integration-focused.
- Expected include roots for integration are:
  - `Core/Inc`
  - `App/Inc`
  - `Drivers/ADC/Inc`
- Before integrating on target hardware, wire the new modules to CubeMX-generated handles, FRAM I/O, relay control, ADC DMA ownership, RTC backup registers, and the USB CDC transport.

## Known assumptions

- FM24CL64 FRAM uses an 8 KiB address space with:
  - header/settings block at `0x0000`
  - bank 1 presets at `0x0100`
  - bank 2 presets at `0x03C0`
  - 176 stepped preset slots per bank, 4 bytes per slot
- Preset record bytes are `cap_mask`, `ind_mask`, `flags`, `valid_marker`.
- TCI remains a placeholder for future expansion and is intentionally not implemented here.
