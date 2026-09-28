# esphome-rfm69-somfy

An ESPHome external component that drives an **Adafruit RFM69HCW 433MHz Radio
FeatherWing** ([product 3230](https://www.adafruit.com/product/3230)) to send
**Somfy RTS** remote-control frames — the protocol used by Somfy-motorized
rollershades, blinds, and awnings (including many SunSetter awnings, which use
Somfy motors under the hood).

## Why this exists

ESPHome has no native RFM69 component, and Somfy RTS isn't packet radio at all —
it's raw OOK (on-off keying) bit-banging at 433.42MHz, a non-standard frequency
most 433MHz modules don't cover. Every existing ESPHome Somfy RTS project targets
an SX127x (LoRa) or CC1101 radio; none target RFM69.

This component puts the RFM69 into direct/continuous OOK mode (so its DIO2 pin
becomes a plain data line the microcontroller drives directly) and reuses the
actual Somfy RTS protocol implementation — frame encoding, rolling codes, RFM69
register setup — from [etimou/SomfyRTS](https://github.com/etimou/SomfyRTS), a
GPL-3.0 Arduino library. That library doesn't build on ESP32 as-is; the vendored
copy under `components/somfy_rts/` patches three concrete portability bugs (see
the file headers in that directory for exactly what and why):

- No ESP32 branch in `RFM69OOK`'s per-MCU default pin macros (undefined identifiers).
- `EEPROM.begin()`/`EEPROM.commit()` were only called on ESP8266, but ESP32's
  Arduino `EEPROM` library needs them too, or the rolling code never reaches flash.
- `select()`/`unselect()` referenced AVR-only `SPCR`/`SPSR` registers on any
  non-ESP8266 target, which doesn't compile on ESP32 either.
- The ESP8266-only `GPOS`/`GPOC` fast register macros don't exist on ESP32; the
  bit-banging falls back to `digitalWrite()` there (negligible overhead against
  Somfy RTS's 640µs-scale symbol timing).

Verified with `esphome compile` against an ESP32-Pico Feather (Adafruit HUZZAH32)
target — see [examples/patio-somfy-bridge.yaml](examples/patio-somfy-bridge.yaml).
This has **not** been tested against real RFM69 hardware or a real shade motor;
treat it as a solid starting point, not a guarantee.

## Hardware

- An ESP32 board (developed against the Adafruit HUZZAH32 / "ESP32-Pico Feather").
- An [Adafruit RFM69HCW 433MHz Radio FeatherWing](https://www.adafruit.com/product/3230)
  stacked on it. **Must be the 433MHz variant** — Somfy RTS's 433.42MHz frequency
  falls within the RFM69's 424–510MHz band option, but not its 868/915MHz one.

### Wiring

SCK/MOSI/MISO are fixed by the stacked-header SPI bus and need no extra wires.
The Radio FeatherWing itself has a labeled row of jumper pads (A–F, plus RX/TX/
SCL/SDA) next to its IRQ/CS/RST pads specifically for this — you solder a short
wire directly between two pads *on the Wing*, not out to bare pins on the Feather
underneath. Confirmed against Adafruit's own EAGLE schematics for both boards
([Radio FeatherWing](https://github.com/adafruit/Adafruit-Radio-FeatherWing-PCB) /
[HUZZAH32](https://github.com/adafruit/Adafruit-HUZZAH32-ESP32-Feather-PCB)), pad
"A" on this Wing is GPIO27 and pad "B" is GPIO33 on the HUZZAH32/ESP32-Pico Feather
specifically (Adafruit doesn't publish this mapping themselves — for ESP32 boards
their guide just says "wire however you like" — so this won't hold for other
Feather boards or other Wing revisions):

| Solder together | Effect |
|---|---|
| Wing's "IRQ" pad ↔ Wing's "A" pad | RFM69 DIO2 (direct-modulation data line) → GPIO27 |
| Wing's "CS" pad ↔ Wing's "B" pad | SPI chip select → GPIO33 |
| RST | *(leave unbridged)* — this driver never resets the radio, and RST has an internal pulldown |

To use different pins, first check which letter pad carries the GPIO you want (the
schematics above are the only reliable source — trace the `MS1`/`JPn` nets in the
Wing's `.sch`, then match by pin position against the target board's own header
part), then fork this repo and edit `SOMFY_RFM69_CS_PIN` / `SOMFY_RFM69_DIO2_PIN`
in `components/somfy_rts/somfy_rts_lib.h` — they're fixed at
compile time because the underlying driver is a single process-wide radio object
(there's only one physical radio per device, so this is fine).

## Usage

See [examples/patio-somfy-bridge.yaml](examples/patio-somfy-bridge.yaml) for a
complete config. The short version:

```yaml
external_components:
  - source: github://chrispyduck/esphome-rfm69-somfy
    components: [somfy_rts]

somfy_rts:
  id: rts_hub

cover:
  - platform: somfy_rts
    name: "Patio Rollershade"
    somfy_rts_id: rts_hub
    remote_number: 0   # each cover needs a distinct number - it's both the RTS
                        # "address" the motor sees and the flash slot its rolling
                        # code is stored in
```

Somfy RTS motors give **no position or state feedback**, so each cover is exposed
as an "assumed state" cover (open/close/stop buttons, no position slider) rather
than one that pretends to track a position it can't verify.

### Pairing a shade or awning

1. On the shade/awning's *existing* remote, hold the PROG button until the motor
   jogs briefly — this puts it into pairing mode.
2. Within a few seconds, send `esphome::somfy_rts::SOMFY_CMD_PROG` for that cover's
   `remote_number` (the example YAML wires this to a per-cover "Pair ..." button).
3. The motor jogs again, confirming the new virtual remote is registered.

Do this once per cover. Re-flashing does **not** undo pairing (the rolling code is
stored in flash), but changing a cover's `remote_number` after pairing makes it act
as a brand new, unpaired remote — the motor will ignore it until paired again.

## License

GPL-3.0 (see [LICENSE](LICENSE)) — inherited from the vendored
[etimou/SomfyRTS](https://github.com/etimou/SomfyRTS) driver this component builds
on, itself based on [Nickduino/Somfy_Remote](https://github.com/Nickduino/Somfy_Remote)
and [kobuki/RFM69OOK](https://github.com/kobuki/RFM69OOK) (RFM69 driver originally
by Felix Rusu / LowPowerLab).
