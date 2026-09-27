# ATU-1k Firmware Architecture

## System Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     STM32F405RGT6                           │
│                     (168 MHz ARM-M4)                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐   │
│  │   USB OTG   │  │   ADC1      │  │   SPI1/SPI3      │   │
│  │   (VCP)     │  │ (FWD/REV)   │  │  (Display/Relay) │   │
│  │ CAT/TCI     │  │ + DMA       │  │                  │   │
│  └─────┬───────┘  └──────┬──────┘  └────────┬─────────┘   │
│        │                 │                   │              │
│        │                 │                   │              │
│   ┌────v─────────────────v───────────────────v──────┐      │
│   │         Main Control Loop                       │      │
│   │  • Frequency tracking (CAT/TCI/Counter)         │      │
│   │  • SWR measurement & smoothing                  │      │
│   │  • Preset lookup & apply                        │      │
│   │  • Auto-tuning algorithm                        │      │
│   │  • UI update (display, buttons)                 │      │
│   └──────────────────────────────────────────────────┘      │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌────────┐  ┌────────────┐   │
│  │   I2C1   │  │  TIM2/3  │  │ GPIO   │  │   UART1    │   │
│  │  (FRAM)  │  │(Counter) │  │(Relay) │  │   (Debug)  │   │
│  └──────────┘  └──────────┘  └────────┘  └────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
         │              │              │            │
         │              │              │            │
      ┌──v──┐    ┌──────v─────┐   ┌───v────┐   ┌───v─────┐
      │FRAM │    │Display/BL  │   │Relays  │   │Buttons/ │
      │8KB  │    │ILI9341     │   │74HC595 │   │Encoder  │
      └─────┘    │240×320     │   │+TD62783    └─────────┘
                 └────────────┘   └────────┘
```

## Core Modules

### 1. **Main Loop** (`main.c`)
- Initialization of all peripherals
- Main control loop (100 ms cycle)
- Frequency source priority logic
- Display update trigger

### 2. **ADC & SWR Measurement** (`Drivers/ADC/`)
- **adc.c**: DMA-based continuous sampling
  - Dual channel (FWD, REV) + VREFINT
  - DMA circular buffer (64 samples per channel)
  - Triggered by TIM3 every 2 ms
- **swr_meter.c**: SWR calculation
  - BAT41 Schottky diode compensation
  - Exponential smoothing (EMA filter)
  - Power calculation (forward/reverse/net)

### 3. **Relay Control** (`Drivers/Relay/`)
- **relay.c**: SPI3 interface to dual 74HC595 shift registers
  - Capacitor matrix (8 values: 5pF to 640pF)
  - Inductor matrix (8 values: 0.05µH to 6.4µH)
  - GPIO direct control (C_in/C_out, Bypass, OUT_PA)
  - Relay settling delay enforcement

### 4. **Display & UI** (`Drivers/Display/`)
- **ili9341.c**: SPI1 interface to ILI9341 controller
  - Landscape orientation (240×320 effectively 320×240 in code)
  - Frame buffering for smooth updates
- **ui.c** (`App/ui.c`): Screen management
  - Status screen (frequency, SWR, power, source indicator)
  - Menu system (settings, calibration)
  - Live tuning progress display

### 5. **FRAM Memory** (`Drivers/FRAM/`)
- **fm24cl64.c**: I2C interface to 8 KB FRAM
  - Preset storage (cap, ind, topology per frequency segment)
  - Service area (settings, checksums)
  - Fast read/write (no erase cycles needed)

### 6. **Frequency Counter** (`Drivers/ADC/` + TIM)
- **adc.c**: TIM3/TIM4 configured for External Clock Mode 2
  - ETR Prescaler = DIV8 (divides 55 MHz to 6.875 MHz)
  - Counter gate: TIM2 pulse every 100 ms
  - Formula: freq_hz = (counter × 8) × 10

### 7. **USB Virtual COM Port** (`Drivers/USB/`)
- **usb_vcp.c**: USB OTG FS CDC class
  - Enumerates as virtual COM port (9600-115200 baud)
  - Circular RX/TX buffers

### 8. **CAT Command Parser** (`App/cat_parser.c`)
- **Yaesu CAT**: FA, IF commands
- **Kenwood CAT**: FA, IF commands
- **Icom CI-V**: Frequency command parsing
- Frequency extraction and timeout handling

### 9. **TCI TCP Client** (`App/` - future)
- TCP/IP socket to TCI server
- JSON-based frequency updates
- Requires external Ethernet/WiFi module (not on current PCB)

### 10. **Tuning Algorithm** (`App/tuner.c`)
- Non-blocking L-network tuning state machine
- Topology selection (IN vs OUT), coarse greedy search, and fine search
- Fast-tune path when a preset is already available
- OUT_PA forced off for the whole tuning window and restored on exit

### 11. **Storage / Persistence** (`App/storage.c`, `App/rtc_backup.c`)
- FRAM settings header + two independent preset banks
- Band ID + stepped frequency offset → linear preset slot
- Explicit preset valid marker for erased/uninitialized detection
- RTC backup snapshot for last frequency + relay state restore after reset

### 12. **UI Input / Menu** (`App/button.c`, `App/menu.c`)
- 5 ms polling debouncer with 7-sample stability target
- 500 ms repeat for C+/C-/L+/L-
- Event-driven 10-item settings menu with FRAM persistence

### 13. **Power Metering** (`Drivers/ADC/power_meter.c`)
- 64-sample/channel FWD/REV/VREFINT batch processing
- Logical ADC swap support
- Forward/reverse/net power and SWR calculation from linear detector scaling

## Interrupt & Timer Structure

| Timer | Function | Frequency |
|-------|----------|----------|
| **TIM2** | ADC gate (frequency counter) | 10 Hz (100 ms) |
| **TIM3** | ADC trigger + Backlight PWM | 500 Hz + PWM |
| **TIM4** | Main loop timing | 10 Hz |
| **DMA1** | ADC buffer circulate | Auto (ADC-driven) |
| **EXTI** | Button interrupts | On press |

## Memory Layout

### STM32F405 Internal Flash
```
0x08000000 ─ 0x08007FFF (32 KB): Bootloader (optional)
0x08008000 ─ 0x080FFFFF (1 MB - 32KB): Firmware code
```

### I2C FRAM (FM24CL64)
```
0x0000 ─ 0x003F: Settings/header block
  magic, version, checksum, persisted menu settings

0x0100 ─ 0x03BF: Preset bank 1 (176 × 4-byte records)
0x03C0 ─ 0x067F: Preset bank 2 (176 × 4-byte records)
  record = cap_mask, ind_mask, flags, valid_marker

0x0680 ─ 0x1FFF: Reserved for future expansion
```

## Data Flow: Frequency Update

```
CAT Command (e.g., FA14200000;)
    ↓
USB VCP RX interrupt
    ↓
cat_parser_process() → extract frequency in Hz
    ↓
g_sys_state.freq_khz = 14200 (kHz)
g_sys_state.freq_source = FREQ_SOURCE_CAT
    ↓
storage.c: find_band() → band_id = BAND_20M
    ↓
storage.c: find_preset() → lookup FRAM in active bank
    ↓
IF FOUND:
    relay_apply_preset()
    display_color = GREEN
ELSE:
    display_color = ORANGE
    (await TUNE button or next CAT update)
    ↓
Display update cycle
    ↓
Screen shows: "14.200 MHz ✓ (green) Source: CAT"
```

## Data Flow: Auto-Tuning

```
Operator presses TUNE button (PC2)
    ↓
button_isr() → set tuner_start_flag = 1
    ↓
Main loop detects flag
    ↓
tuner_start_auto_tune()
    ├─ Step 1: Topology selection (1 s)
    ├─ Step 2: Coarse inductance (1 s)
    ├─ Step 3: Coarse capacitance (1 s)
    ├─ Step 4: Fine search (3-10 s)
    └─ Step 5: Finalize without auto-save
    ↓
Display shows result (GREEN if SWR < threshold, RED if timeout)
    ↓
Operator can:
   • Keep result active without saving
   • Long-press TUNE to save current preset explicitly
   • Manually adjust with C±, L± buttons
   • Change frequency and tune new frequency
```

## USB Connection Logic

```
USB Cable Connected (5V on PA9)
    ↓
gpio_usb_detect_read() = 1
    ↓
g_sys_state.usb_present = 1
g_sys_state.freq_source_priority = CAT > Counter
    ↓
Transistor key BLOCKS PD2 input (frequency counter inactive)
    ↓
Await CAT commands on USB VCP; PD2 counter is considered unavailable while USB is present

─────────────────────────────────────

USB Cable Disconnected (PA9 = LOW)
    ↓
g_sys_state.usb_present = 0
g_sys_state.freq_source_priority = Counter
    ↓
Transistor key OPENS PD2 input (frequency counter active)
    ↓
TIM3 counts RF signal on PD2 every 100 ms
    ↓
Automatic frequency tracking from RF
```

## Error Handling

| Error | Detection | Action |
|-------|-----------|--------|
| **ADC Overrange** | FWD > 4000 counts | Skip measurement, display warning |
| **No RF (FWD < MIN)** | FWD < 35 counts | Return SWR = 1.0, disable tuning |
| **Tuning Timeout** | elapsed > timeout_ms | Use best result found, display "TIMEOUT" |
| **Preset Not Found** | FRAM read returns 0xFF | Display orange, await TUNE |
| **FRAM Write Error** | I2C NACK | Retry 3×, log error to UART |
| **USB Enumerate Fail** | USB timeout | Default to Frequency Counter mode |
| **Button Stuck** | Same button > 5 s | Debounce timeout, clear flag |

---

## Future Enhancements

1. **TCI Support**: Add Ethernet/WiFi module
2. **Remote Control**: Web UI for tuning parameters
3. **Multi-band Optimization**: Neural network for faster coarse search
4. **Impedance Display**: Calculate Z from advanced phase detection
5. **Logging**: SD card for tuning history
