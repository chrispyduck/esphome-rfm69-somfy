# esphome-rfm69-somfy

An ESPHome external component that drives an **Adafruit RFM69HCW 433MHz Radio
FeatherWing** ([product 3230](https://www.adafruit.com/product/3230)) to control
motorized shades over 433MHz, with no native support in ESPHome for either radio
usage this repo needs:

- **Somfy RTS** (`somfy_rts`) — the protocol used by Somfy-motorized rollershades,
  blinds, and awnings (including many SunSetter awnings, which use Somfy motors
  under the hood). Continuous OOK at 433.42MHz, decoded frames, rolling codes.
- **Allesin RTS** (`allesin_rts`) — the protocol used by Allesin M130 roller blind
  motors (remote AC2101-01A), also sold under other brand names on the same
  SunfreeMotor hardware. Continuous **FSK** at 433.914MHz, **replayed** rather than
  decoded - see [Allesin RTS](#allesin-rts) below for what that means and its much
  larger set of caveats.

Both protocols direct-modulate the *same* physical RFM69 chip (see
`components/rfm69_direct/`) - one radio, one set of wiring, either or both
protocols in one ESPHome config.

## Why this exists

ESPHome has no native RFM69 component, and Somfy RTS isn't packet radio at all —
it's raw OOK (on-off keying) bit-banging at 433.42MHz, a non-standard frequency
most 433MHz modules don't cover. Every existing ESPHome Somfy RTS project targets
an SX127x (LoRa) or CC1101 radio; none target RFM69.

This component puts the RFM69 into direct/continuous OOK mode (so its DIO2 pin
becomes a plain data line the microcontroller drives directly) and reuses the
actual Somfy RTS protocol implementation — frame encoding, rolling codes, RFM69
register setup — from [etimou/SomfyRTS](https://github.com/etimou/SomfyRTS), a
GPL-3.0 Arduino library. That library was written for AVR/ESP8266 and doesn't work
on ESP32 as-is; the vendored copy under `components/somfy_rts/` patches the
following (see the file headers in that directory for exactly what and why):

- No ESP32 branch in `RFM69OOK`'s per-MCU default pin macros (undefined identifiers).
- `select()`/`unselect()` referenced AVR-only `SPCR`/`SPSR` registers on any
  non-ESP8266 target (doesn't compile on ESP32), and disabled interrupts around
  every SPI register access (pointless on ESP32's atomic hardware SPI).
- The ESP8266-only `GPOS`/`GPOC` fast register macros don't exist on ESP32; the
  bit-banging falls back to `digitalWrite()` (negligible overhead against Somfy
  RTS's 640µs-scale symbol timing).
- **`SPI.begin()` with no arguments uses generic default pins (SCK=18, MISO=19,
  MOSI=23) that match no Feather - on the Adafruit ESP32 Feather V2 it never
  returns, which hangs `setup()`, trips the watchdog, and boot-loops the board
  into ESPHome's safe mode.** The SPI pins are now passed explicitly.
- `initialize()` spun forever waiting on the radio. It now checks the chip's
  `RegVersion` register (0x24 on a healthy RFM69) and uses a bounded wait, so a
  missing or miswired radio logs an error and marks the component failed instead
  of hanging boot.
- `sendSomfy()` held interrupts disabled for ~500ms across all three frames; each
  frame now gets its own shorter critical section.
- Rolling codes are persisted through ESPHome's own preferences system instead of
  Arduino's `EEPROM` library (see "Rolling-code storage" below).

Tested on an **Adafruit ESP32 Feather V2 + RFM69HCW 433MHz FeatherWing**: it boots
cleanly, the radio answers on SPI (`RegVersion=0x24`), and the example config builds
and runs - see [examples/patio-somfy-bridge.yaml](examples/patio-somfy-bridge.yaml).
**Not yet verified:** that an actual RF frame leaves the radio and moves a real
shade/awning motor. Treat it as a solid starting point, not a guarantee.

## Hardware

- An [Adafruit ESP32 Feather V2](https://www.adafruit.com/product/5400)
  (ESP32-PICO-V3-02); use `board: adafruit_feather_esp32_v2`. Its stacked-header SPI
  pins (SCK=5, MISO=21, MOSI=19) are what this component uses by default. Other
  Feathers differ (e.g. the HUZZAH32 is SCK=5, MISO=19, MOSI=18) - override
  `RF69OOK_SPI_SCK`/`_MISO`/`_MOSI` (see `RFM69OOK.h`).
- An [Adafruit RFM69HCW 433MHz Radio FeatherWing](https://www.adafruit.com/product/3230)
  stacked on it. **Must be the 433MHz variant** — Somfy RTS's 433.42MHz frequency
  falls within the RFM69's 424–510MHz band option, but not its 868/915MHz one.

### Wiring

SCK/MOSI/MISO are fixed by the stacked-header SPI bus and need no extra wires.
The Radio FeatherWing itself has a labeled row of jumper pads (A–F, plus RX/TX/
SCL/SDA) next to its IRQ/CS/RST pads specifically for this — you solder a short
wire directly between two pads *on the Wing*, not out to bare pins on the Feather
underneath. Adafruit doesn't publish the letter-to-GPIO mapping for ESP32 boards
(their guide just says "wire however you like"), so it was traced through their EAGLE
schematics ([Radio FeatherWing](https://github.com/adafruit/Adafruit-Radio-FeatherWing-PCB),
[HUZZAH32](https://github.com/adafruit/Adafruit-HUZZAH32-ESP32-Feather-PCB)): pad "A"
is the Feather header's GPIO27 position and pad "B" is GPIO33. The Feather V2 keeps
the same header order for those positions, and **pad B → CS = GPIO33 is confirmed on
real hardware** (the radio answers on SPI with that jumper in place). Pad A → DIO2 =
GPIO27 follows from the same mapping but isn't yet confirmed by an actual
transmission. This mapping is specific to these boards - it won't hold for other
Feathers or Wing revisions:

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
complete config covering both protocols. The short version:

```yaml
external_components:
  - source: github://chrispyduck/esphome-rfm69-somfy
    # rfm69_direct is the shared radio driver both protocols below sit on top of -
    # list it even if you only want one of the two.
    components: [rfm69_direct, somfy_rts, allesin_rts]

somfy_rts:
  id: rts_hub

allesin_rts:
  id: allesin_hub

cover:
  - platform: somfy_rts
    name: "Patio Rollershade"
    somfy_rts_id: rts_hub
    remote_number: 0   # each cover needs a distinct number - it's both the RTS
                        # "address" the motor sees and the flash slot its rolling
                        # code is stored in
  - platform: allesin_rts
    name: "Allesin Blind"
    allesin_rts_id: allesin_hub
```

Both motors give **no position or state feedback**, so each cover is exposed as an
"assumed state" cover (open/close/stop buttons, no position slider) rather than one
that pretends to track a position it can't verify.

### Rolling-code storage

Each remote's rolling code lives in ESPHome's preferences system, which keeps it in
RAM and flushes to flash every 60 seconds by default (`preferences:
flash_write_interval`). If the device loses power within that window of a command,
the stored counter can lag the last transmitted one by a few; the motor then ignores
the next command or two (it rejects codes that aren't ahead of the last one it saw)
until the counter catches back up. If a cover stops responding entirely after a
firmware update that changed storage, re-pair it.

### Pairing a shade or awning

1. On the shade/awning's *existing* remote, hold the PROG button until the motor
   jogs briefly — this puts it into pairing mode.
2. Within a few seconds, send `esphome::somfy_rts::SOMFY_CMD_PROG` for that cover's
   `remote_number` (the example YAML wires this to a per-cover "Pair ..." button).
3. The motor jogs again, confirming the new virtual remote is registered.

Do this once per cover. Re-flashing does **not** undo pairing (the rolling code is
stored in flash), but changing a cover's `remote_number` after pairing makes it act
as a brand new, unpaired remote — the motor will ignore it until paired again.

## Allesin RTS

This protocol is **not decoded** - no one has published a byte/command/checksum
layout for it. `allesin_rts` instead **replays** one captured remote's RF signal
verbatim: carrier, FSK deviation, and bit timing all measured off the air with a
HackRF by Home Assistant Community forum user MartinsZB and published at
<https://community.home-assistant.io/t/allesin-m130-roller-blinds/697463/15>. See
[components/allesin_rts/allesin_signals.h](components/allesin_rts/allesin_signals.h)
for the raw data and [allesin_rts_lib.h](components/allesin_rts/allesin_rts_lib.h)
for what is/isn't known about the signal's structure.

What that means in practice:

- **No `remote_number`.** The captured signal carries no known addressing, so
  every `allesin_rts` cover you configure sends the *exact same* RF bytes. If you
  have more than one Allesin shade in range, commanding one from Home Assistant
  will currently move all of them - there's no way to target just one until
  someone decodes (or captures a second, distinct) addressing field.
- **No pairing step.** Unlike Somfy RTS, there's no known PROG/learn command for
  this protocol. The captured signal already matches a real AC2101-01A remote, so
  if your blind's receiver accepts *any* transmitter with that remote's code
  (the forum post implies this is shared across many rebranded units, not
  unique per remote), it should just work with no pairing at all. If your unit
  is paired specifically to its own factory remote and rejects others, this
  won't move it, and there's currently no documented way to re-pair it to accept
  this signal.
- **No rolling code.** The signal is static - replaying it is deterministic and
  doesn't depend on any state this device has seen before (unlike Somfy's rolling
  counter).
- **Untested against real hardware by this repo.** The FSK tone polarity
  (`DIO2` high = `f_c + deviation` vs `f_c - deviation`) is an assumption based on
  the RFM69's usual continuous-mode behavior, not confirmed on an Allesin motor.
  If a blind doesn't respond, that polarity is the first thing worth flipping
  (swap the `HIGH`/`LOW` branches in `playRuns()` in
  [allesin_rts_lib.cpp](components/allesin_rts/allesin_rts_lib.cpp)) before
  assuming the capture itself doesn't apply to your unit.

## License

GPL-3.0 (see [LICENSE](LICENSE)) — inherited from the vendored
[etimou/SomfyRTS](https://github.com/etimou/SomfyRTS) driver `somfy_rts` and
`rfm69_direct` build on, itself based on
[Nickduino/Somfy_Remote](https://github.com/Nickduino/Somfy_Remote) and
[kobuki/RFM69OOK](https://github.com/kobuki/RFM69OOK) (RFM69 driver originally by
Felix Rusu / LowPowerLab).

The captured waveform data in
[components/allesin_rts/allesin_signals.h](components/allesin_rts/allesin_signals.h)
is third-party data, not code written for this repo: it was captured and published
publicly by Home Assistant Community forum user MartinsZB (see the
[Allesin RTS](#allesin-rts) section above for the source thread/post). It's
included here for interoperability under the same spirit the original post shared
it in; no additional license is asserted over that data beyond what its author
already granted by posting it publicly.
