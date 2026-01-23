# Hardware Validation Guide

This document provides step-by-step instructions for validating newly assembled ESP32-C3 Relay Module boards using the serial test commands.

## Prerequisites

1. ESP-IDF development environment installed (v5.1.x or v5.2.x)
2. USB cable (USB-C or USB-A to USB-C depending on board revision)
3. Serial terminal application (ESP-IDF Monitor, PuTTY, minicom, or similar)

## Step 1: Flash Firmware

```bash
cd Firmware/esp32c3-relay-firmware
idf.py build
idf.py flash
```

Wait for the flash to complete. The board will automatically reset.

## Step 2: Connect Serial Terminal

Connect to the board at **115200 baud, 8N1** (8 data bits, no parity, 1 stop bit).

Using ESP-IDF Monitor:
```bash
idf.py monitor
```

Using other terminal applications:
- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

## Step 3: Verify Serial Communication

Send the PING command to verify serial communication is working:

```
> PING
< OK|pong
```

**Expected result:** Response `OK|pong` confirms serial link is operational.

**If no response:**
- Check USB cable connection
- Verify correct COM port/device selected
- Confirm baud rate is 115200
- Try pressing reset button on the board

## Step 4: Test Relay

Test relay control with audible verification:

```
> TEST_RELAY ON
< OK|relay_on
```

**Expected result:**
- Response `OK|relay_on`
- Audible click from relay
- Relay LED indicator (if present) illuminates

```
> TEST_RELAY OFF
< OK|relay_off
```

**Expected result:**
- Response `OK|relay_off`
- Audible click from relay
- Relay returns to de-energized state

**Hardware validation:** GPIO7 controls relay coil through Q1 MOSFET driver.

## Step 5: Test Status LED (White)

```
> TEST_LED STATUS ON
< OK|led_on
```

**Expected result:** White LED D3 illuminates

```
> TEST_LED STATUS OFF
< OK|led_off
```

**Expected result:** White LED D3 turns off

```
> TEST_LED STATUS BLINK
< OK|led_blinking
```

**Expected result:** White LED D3 blinks at 1-second intervals (slow blink pattern)

**Hardware validation:** GPIO10 controls white status LED D3 (active high, R8 1kΩ current limiter).

## Step 6: Test Error LED (Red)

```
> TEST_LED ERROR ON
< OK|led_on
```

**Expected result:** Red LED D1 illuminates

```
> TEST_LED ERROR OFF
< OK|led_off
```

**Expected result:** Red LED D1 turns off

**Hardware validation:** GPIO0 controls red error LED D1 (active high, R7 1kΩ current limiter).

**Note:** GPIO0 is a strapping pin. LED circuit doesn't pull GPIO low, so it's safe to use.

## Step 7: Test Button Input

With button released:
```
> TEST_BUTTON
< OK|button_state|released
```

Press and hold the button, then send command:
```
> TEST_BUTTON
< OK|button_state|pressed
```

**Expected result:** Response correctly reflects button state.

**Hardware validation:** GPIO9 reads button S1 state (active low with R6 10kΩ pull-up).

## Step 8: Test Help Command

```
> HELP
< OK|help
< Available commands:
<   PING - Test connectivity (responds: pong)
<   TEST_RELAY [ON|OFF] - Control relay
<   TEST_LED [STATUS|ERROR] [ON|OFF|BLINK] - Control LEDs
<   TEST_BUTTON - Read button state
<   HELP - Show available commands
```

## Step 9: Test Error Handling

Invalid command:
```
> FOOBAR
< ERROR|invalid_command|Unknown command. Type HELP for available commands.
```

Invalid argument:
```
> TEST_RELAY FOO
< ERROR|invalid_argument|Relay state must be ON or OFF
```

## Step 10: Dual Power Supply Validation (AC10)

The board supports two power sources with Schottky diode isolation:

### Test 1: USB-C Power Only
1. Connect only USB-C cable
2. Verify firmware boots (status LED blinks)
3. Run PING command to confirm operation
4. Run all test commands

### Test 2: External DC Power Only
1. Disconnect USB-C
2. Connect external DC 12-24V power supply
3. Connect USB cable for serial (data only, board powered externally)
4. Verify firmware boots (status LED blinks)
5. Run all test commands

### Test 3: Both Power Sources (Schottky Isolation)
1. Connect USB-C cable
2. Also connect external DC 12-24V power
3. Verify firmware continues operating (no conflict due to Schottky diode isolation)
4. Run all test commands
5. Disconnect USB-C while external power remains - board should continue operating
6. Reconnect USB-C while external power remains - board should continue operating

**Pass criteria:** Board operates correctly with either or both power sources connected.

## Complete Validation Checklist

| Test | Command | Expected Response | Hardware Behavior | Pass |
|------|---------|-------------------|-------------------|------|
| Serial connectivity | `PING` | `OK|pong` | - | [ ] |
| Relay ON | `TEST_RELAY ON` | `OK|relay_on` | Audible click | [ ] |
| Relay OFF | `TEST_RELAY OFF` | `OK|relay_off` | Audible click | [ ] |
| Status LED ON | `TEST_LED STATUS ON` | `OK|led_on` | White LED on | [ ] |
| Status LED OFF | `TEST_LED STATUS OFF` | `OK|led_off` | White LED off | [ ] |
| Status LED BLINK | `TEST_LED STATUS BLINK` | `OK|led_blinking` | White LED blinks | [ ] |
| Error LED ON | `TEST_LED ERROR ON` | `OK|led_on` | Red LED on | [ ] |
| Error LED OFF | `TEST_LED ERROR OFF` | `OK|led_off` | Red LED off | [ ] |
| Button released | `TEST_BUTTON` | `OK|button_state|released` | - | [ ] |
| Button pressed | `TEST_BUTTON` (hold button) | `OK|button_state|pressed` | - | [ ] |
| Help command | `HELP` | `OK|help` + command list | - | [ ] |
| Invalid command | `FOOBAR` | `ERROR|invalid_command|...` | - | [ ] |
| Invalid argument | `TEST_RELAY FOO` | `ERROR|invalid_argument|...` | - | [ ] |
| USB power only | - | - | Board operates | [ ] |
| DC power only | - | - | Board operates | [ ] |
| Both power sources | - | - | Board operates | [ ] |

## Troubleshooting

### No serial response
- Check USB cable connection
- Verify COM port settings (115200, 8N1)
- Press reset button and wait for boot messages
- Check if correct USB driver is installed (CP2102 or CH340)

### Relay doesn't click
- Verify power supply provides sufficient current
- Check Q1 MOSFET solder joints
- Measure GPIO7 voltage (should switch between 0V and 3.3V)

### LED doesn't illuminate
- Check LED orientation (correct polarity)
- Verify current limiting resistor is correct value
- Measure GPIO voltage at LED cathode

### Button state incorrect
- Check pull-up resistor R6 (10kΩ)
- Verify button solder joints
- Measure GPIO9 voltage (should be high when released, low when pressed)

## GPIO Reference

| GPIO | Function | Active State | Notes |
|------|----------|--------------|-------|
| GPIO7 | Relay control | HIGH = ON | Through Q1 MOSFET driver |
| GPIO9 | Button input | LOW = pressed | External pull-up R6 10kΩ |
| GPIO10 | Status LED (white) | HIGH = ON | Current limit R8 1kΩ |
| GPIO0 | Error LED (red) | HIGH = ON | Current limit R7 1kΩ, strapping pin |
