# UART Protocol Notes

This document captures what we have discovered so far about the UART link between the Wi‑Fi/BT bridge (“master”) and the ceiling‑fan controller (“slave”). All byte values are hex unless stated otherwise.

---

## Serial Link Basics

- Baud rate: **9600 8N1**
- Messages are simple byte streams; no escaping has been observed.
- Both directions use a **0x20** start byte followed by fixed‑length frames:
  - Master → Slave: **5 bytes** (`0x20` + 4‑byte payload)
  - Slave → Master: **12 bytes** (see layout below)
- **Checksums** are a simple **XOR of all preceding bytes** (including the 0x20 start). We previously assumed a sum-of-bytes checksum, but every verified master/slave frame in the captures conforms to the XOR rule.

---

## Master Frames

### Keep‑Alive

```
20 01 00 00 21
```

- Sent every ≈0.2 s (~5 Hz) continuously.
- Serves as a heartbeat; no payload semantics beyond “still alive”.

### Command Map

| Command | Frame format (`20 cmd HH LL CC`) | Meaning |
|---------|----------------------------------|---------|
| `0x01`  | `20 01 00 00 21`                | Keep‑alive (see above) |
| `0x02`  | Payload high byte encodes the **fan level**, snapped to one of the discrete codes (`0x00/0x10/0x21/0x32/0x42/0x53/0x64`). Low byte stays `0x00`. | Set fan speed |
| `0x03`  | Payload high byte = **brightness %** (0‑100). Low byte is `0x00`. | Set light brightness |
| `0x04`  | `HH` = direction (0=forward, 1=reverse); low byte `0x00`. Examples: `20 04 01 00 25` reverse, `20 04 00 00 24` forward. | Set fan direction |
| `0x08`  | Payload high byte = **color temperature step** (`0x01`=2700 K … `0x06`=6500 K). Low byte `0x00`. | Set light color temperature |
| `0x09`…`0x0E` | Seen only during boot; assumed device init queries. |

> Checksums are `start ^ command ^ HH ^ LL`.  
> Example: `20 02 32 00 10` → `0x20 ^ 0x02 ^ 0x32 ^ 0x00 = 0x10`.

### Boot / Recovery Sequence

Capture `capture_20251024_182648.jsonl` contains three power cycles and finally exposed what the boot-only opcodes do:

#### Master boot burst

- Roughly **2.4 s after power is applied** the master fires a fixed burst of commands with zeroed payloads:

  | Order | Frame | Notes |
  |-------|-------|-------|
  | 1 | `20 09 00 00 29` | Boot opcode `0x09` (purpose unknown) |
  | 2 | `20 0A 00 00 2A` | Boot opcode `0x0A` |
  | 3 | `20 0B 00 00 2B` | Boot opcode `0x0B` |
  | 4 | `20 0C 00 00 2C` | Boot opcode `0x0C` |
  | 5 | `20 0E 00 00 2E` | Boot opcode `0x0E` (there is no `0x0D` in the burst) |
  | 6 | `20 02 32 00 10` | Resend last fan speed (level 3 / 50 %) |
  | 7 | `20 03 39 00 1A` | Resend last brightness (57 %) |
  | 8+ | `20 01 00 00 21` | Keep‑alives resume at ~5 Hz |

  The order is identical for both successful restarts in the capture, suggesting these packets are a deterministic re‑init script before the bridge trusts status telemetry again.

#### Slave boot frames

- The slave answers with two distinctive frames before it returns to regular telemetry:

  | Status | Stage byte | Frame | Interpretation |
  |--------|------------|-------|----------------|
  | `0x09` | `0x40` (`forward_to_reverse`) | `20 09 06 2F 20 0A 01 2B 20 0B 4C 44` | Announces reboot start; bytes 2‑9 echo the master opcodes and checksums (likely an internal log of the burst) while the stage flags a transitional state. |
  | `0x0C` | `0x0B` (`reboot_stage2`) | `20 0C 06 2A 20 0E 0C 8C 0A B8 0B AC` | Second reboot handshake; continues mirroring earlier opcodes and keeps the stage in a non-directional reboot marker. |
  | `0x02` → `0x03` → `0x01` | `0x00` (`forward_idle`) | `20 02 00 00 32 39 04 00 00 00 2D`, `20 03 … 2C`, `20 01 … 2E` | Sequence that clears the reboot condition, reasserts the actual fan/light state, and then re-enters steady-state reporting.

- These handshake frames share the same payloads each time, which means they can act as reliable markers for “controller just powered up / recovered.”

#### Recovery lockout via rapid toggles

- The second power cycle in the capture (power re-applied at 18:27:26) produced **no UART traffic for 65 s** from either direction. Only after a third quick toggle did the master resume the boot burst and the slave repeat its reboot banners.
- That pattern implies a built-in recovery mode that deliberately keeps the bus muted until another power cycle occurs (or a timeout expires). Rapidly toggling the supply thus seems to arm and then clear the mute window, which is useful to keep in mind while capturing future traces.

---

## Slave Frames (RF Receiver / Fan Controller)

Every 12‑byte frame has the same structure:

```
byte0  : 0x20 (start)
byte1  : status (usually 0x01)
byte2  : RF payload slot (0x00 unless relaying a remote packet)?
byte3  : RF payload slot (mirrors byte2 usage)?
byte4  : fan code
byte5  : brightness (%)
byte6  : color code
byte7  : timer high byte
byte8  : timer low byte
byte9  : RF payload slot / handshake byte (0x00 during steady state)?
byte10 : stage / progress byte (direction state & reboot markers; see below)
byte11 : checksum = XOR(bytes 0‑10)
```

### Fan Output (byte 4)

- The slave reports a discrete **level code**, not an arbitrary percentage. The code is always snapped to one of the observed values below (0 = off):

| Fan code | Level | Percent |
|----------|-------|---------|
| `0x00`   | 0     | 0 % (off) |
| `0x10`   | 1     | 16 % |
| `0x21`   | 2     | 33 % |
| `0x32`   | 3     | 50 % |
| `0x42`   | 4     | 66 % |
| `0x53`   | 5     | 83 % |
| `0x64`   | 6     | 100 % |

- Sending any intermediate “percent” over UART simply causes the controller to round to the nearest level. In practice the bridge only transmits these canonical codes, so mimic tooling should do the same.


### Brightness (byte 5)

Direct percentage (0‑100). `0x00` indicates the light is off.

### Color Code (byte 6)

| Code | Kelvin |
|------|--------|
| `0x01` | 2700 K |
| `0x02` | 3000 K |
| `0x03` | 3500 K |
| `0x04` | 4000 K |
| `0x05` | 5000 K |
| `0x06` | 6500 K |

### Timer (bytes 7–8)

Little‑endian number of **minutes**. Example frames:

| Frame bytes (timer field highlighted) | Meaning |
|---------------------------------------|---------|
| `20 … 10 14 04 **00 78** 00 00 59` | 0x0078 → 120 min (2 h timer) |
| `20 … 10 14 04 **00 F0** 00 00 D1` | 0x00F0 → 240 min (4 h timer) |
| `20 … 10 14 04 **01 E0** 00 00 C0` | 0x01E0 → 480 min (8 h timer) |
| `20 … 10 14 04 **00 00** 00 00 21` | 0 min → timer off |

### Example Decode

```
20 01 00 00 53 50 03 00 78 00 00 89
└─┬─┘ └┬┘ └┬┘ └┬┘ └─ fan 83% (level 5)
  │    │    │    └─ brightness 0x50 (80%)
  │    │    └────── color 0x03 (3500K)
  │    └─────────── timer 0x0078 → 120 min
  └──────────────── checksum = 0x89
```

### Status & Stage Bytes

- **Status byte (byte 1)**  
  `0x01` = steady/idle, `0x04` = change in progress (possibly just 0x04 command in progress), `0x09` = reboot handshake start, `0x0C` = reboot handshake stage 2, `0x02/0x03` = final reboot cleanup frames.

- **Stage byte (byte 10)** (use the top two bits to interpret direction):

| Stage | High bits | Direction meaning | Notes |
|-------|-----------|-------------------|-------|
| `0x00` | `00xxxxxx` | Forward idle / change complete | Spinner clears when going reverse→forward |
| `0x40` | `01xxxxxx` | Forward → Reverse transition | Status often `0x04` during this burst |
| `0x80` | `10xxxxxx` | Reverse idle / change complete | Spinner clears when going forward→reverse |
| `0xC0` | `11xxxxxx` | Reverse → Forward transition (reverse idle just prior to the drop to `0x00`) | Observed during RF remote flips |
| `0x0A`, `0x33`, `0x0B` | — | Reboot handshake stages | Appear with status `0x0B/0x0C/0x02/0x03` while the controller resets |

The decoder uses these bytes to flag `fan_direction_status = forward/reverse` once the stage settles on `0x00` or `0xC0`. Between those steady states, the stage byte announces the active transition, which is what the mobile app uses to manage its “please wait” spinner.

---

## Observed RF Remote Behavior

- All remote button presses (fan speed, brightness, color, timer) originate on the slave. The master only receives these 12‑byte packets and does not transmit additional commands unless it needs to mirror the change.
- When the RF remote adjusts brightness or color, the fan code stays fixed while bytes 5‑6 change.
- Timer changes only affect bytes 7‑8; the master does not send a matching command unless a cloud/app update occurs afterward.

---

## Open Questions

- Boot/init commands (`0x09–0x0E`) act as a deterministic power-up script, but their individual roles remain unknown.
- Slave “status” byte (byte 1) has been constant but may encode fault states.
- We have not yet seen the slave initiate multi-byte responses longer than 12 bytes; it is possible there is an extended format for diagnostics.

---

_Last updated: 2025-10-24_
