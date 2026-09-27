# ATU-1k Auto-Tuning Algorithm

## Overview

The ATU-1k uses a 5-step L-network (Г-цепочка) tuning algorithm with two phases:
1. **Topology Selection** - determine if network should be IN (C_in) or OUT (C_out)
2. **Coarse Search** - binary search for inductance and capacitance rough values
3. **Fine Search** - linear search around optimum for SWR minimum

## Algorithm Steps

### Step 1: Topology Detection (Test Capacitor Sweep)

**Objective:** Determine L-network configuration (IN vs OUT) by measuring SWR with fixed 10 pF capacitor.

**Procedure:**
1. Reset all relays (cap_mask=0, ind_mask=0)
2. Enable test capacitor (10 pF = bit 1)
3. Set C_in/C_out = IN (0), measure SWR → swr_in
4. Set C_in/C_out = OUT (1), measure SWR → swr_out
5. Select topology with lower SWR
6. Fix topology (C_in/C_out state) for remaining steps
7. Save test capacitor state (cap_mask_fixed = 10 pF)

**Duration:** ~1 second (includes relay settling)

---

### Step 2: Coarse Inductance Search

**Objective:** Find major inductor values that improve SWR.

**Procedure:**
1. Start with best_ind = 0
2. For each bit [7..0] (from largest to smallest inductor):
   - Set test_ind = best_ind | (1 << bit)
   - Apply relay mask
   - Wait relay_delay_ms for settling
   - Measure SWR
   - If SWR improved: keep bit set, best_ind = test_ind
   - If SWR not improved: clear bit, best_ind &= ~(1 << bit)
3. Save best_ind as ind_mask_coarse

**Logic:** Sequential bit-by-bit inclusion/exclusion (greedy search)

**Duration:** ~10 relay changes × relay_delay ≈ 1 second

---

### Step 3: Coarse Capacitance Search

**Objective:** Add capacitor values to fine-tune impedance (similar to Step 2).

**Procedure:**
1. Start with best_cap = cap_mask_fixed (from Step 1)
2. For each bit [7..0]:
   - Set test_cap = best_cap | (1 << bit)
   - Apply relay mask
   - Measure SWR
   - Keep or reject bit based on SWR improvement
3. Save best_cap as cap_mask_coarse

**Duration:** ~8-10 relay changes ≈ 1 second

---

### Step 4: Fine Search (±3 bits Linear Sweep)

**Objective:** Find global SWR minimum in narrow sector around coarse optimum.

**Search Space:**
- Capacitor: [cap_coarse - 3, cap_coarse + 3]
- Inductor: [ind_coarse - 3, ind_coarse + 3]
- Total combinations: 7 × 7 = 49 test points

**Procedure:**
1. Initialize best_swr = current_swr, best_cap/ind from Step 3
2. For cap_delta in [-3, -2, -1, 0, 1, 2, 3]:
   - For ind_delta in [-3, -2, -1, 0, 1, 2, 3]:
     - test_cap = cap_coarse + cap_delta (clip to 0-255)
     - test_ind = ind_coarse + ind_delta (clip to 0-255)
     - Check forbidden combinations (skip if needed)
     - Apply relay mask
     - Measure SWR
     - **Early exit:** if SWR < swr_threshold → stop immediately
     - Otherwise: update best if improved
     - Check timeout
3. Save final best_cap, best_ind

**Duration:** Typically 3-10 seconds (depends on # of valid combinations and early exit)

---

### Step 5: Result Finalization

**Objective:** Keep the best tested relay state active, restore PA output, and display outcome.

**Procedure:**
1. Apply final relay masks (already set from Step 4)
2. Do **not** save automatically. Preset storage is operator-driven only:
   - short **TUNE** = start/continue tuning
   - long **TUNE** (>= 500 ms) = save current cap_mask, ind_mask, topology, and bypass state
3. Output result to display:
   - Final SWR value
   - Capacitor and inductor states
   - Status message (OK, TIMEOUT, FAILED)
4. Enable PA output (OUT_PA = 1)

---

## Fast Tuning Mode (Pre-Stored Preset)

If FRAM already contains preset for current frequency:

1. Load preset: cap_mask, ind_mask, topology
2. Apply relay masks immediately
3. Measure SWR
4. **Fine search only** (skip Steps 1-3):
   - Search in ±2 bit range around loaded values
   - Typically completes in 1-2 seconds
5. Keep the refined relay state active, but do **not** write it back to FRAM automatically

---

## Protection & Constraints

### Fine Search Coverage

All clipped integer mask combinations inside the fine-search window are valid. There are no forbidden LC combinations in the current firmware search model.

### Tuning Parameters (Configurable in Menu)

| Parameter | Default | Adjustable |
|-----------|---------|------------|
| SWR Threshold | 1.5 | Yes |
| Relay Delay (ms) | 100 | Yes |
| Timeout (s) | 30 | Yes |
| Test Capacitor | 10 pF | Fixed |
| Fine Search Range | ±3 bits | Fixed |

---

## Frequency Source Priority

1. **USB Connected + CAT Data Available** → Use CAT frequency
2. **USB Disconnected** → Use Frequency Counter (PD2)
3. **USB Connected but CAT stale/invalid** → Manual mode (PD2 counter is blocked while USB is present)
4. **Both Unavailable or Unsupported Band** → Manual mode (operator uses TUNE button)

### Auto-Preset Application

When new frequency received:
```
if (frequency_changed) {
    if (find_preset_in_fram(frequency_khz)) {
        apply_preset();              // Automatic
        display_color = GREEN;       // Indicate found
    } else {
        display_color = ORANGE;      // Indicate not found
        // Await manual TUNE button or CAT command
    }
}
```

---

## Timing Summary

| Phase | Typical Duration | Variability |
|-------|------------------|-------------|
| Step 1 (Topology) | ~1 s | Fixed |
| Step 2 (Coarse L) | ~1 s | Fixed |
| Step 3 (Coarse C) | ~1 s | Fixed |
| Step 4 (Fine Search) | 2-10 s | High (depends on start point) |
| **Total (Full)** | **5-13 s** | High |
| **Fast (Preset)** | **1-2 s** | Low |

---

## Display Status

```
During Tuning:
┌──────────────────────────────┐
│ TUNING... 14.200 MHz         │
│ Step: 4/5 (Fine Search)      │
│ Current SWR: 1.23            │
│ Best: 1.12                   │
│ Elapsed: 8s / Timeout: 30s   │
└──────────────────────────────┘

Tuning Complete:
┌──────────────────────────────┐
│ 14.200 MHz ✓ TUNED           │
│ SWR: 1.15:1  FWD: 100W       │
│ C: [██░░░░░░] L: [██░░░░░░]  │
│ Source: CAT                  │
└──────────────────────────────┘
```

---

## Error Handling

**Timeout During Tuning:**
- Stop current step
- Use best result found so far
- Display message: "TUNING TIMEOUT - using best estimate"
- Enable PA output

**No Valid Combinations Found:**
- Reset all relays
- Display: "TUNING FAILED - antenna mismatch"
- Operator must manually adjust antenna or try different band

**ADC Saturation (FWD too high):**
- Skip measurement
- Display: "ADC OVERRANGE - reduce power"
