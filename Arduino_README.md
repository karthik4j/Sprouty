# Sprouty ESP8266 Firmware

This folder contains the ESP8266 firmware used by the Sprouty precision agriculture monitoring system.

The firmware is designed for an **ESP8266 NodeMCU** and handles sensor readings, MQTT communication, irrigation control, LCD output, and device status reporting.

---

## 1. Firmware Responsibilities

The ESP8266 is responsible for:

- Connecting to Wi-Fi
- Connecting securely to HiveMQ Cloud using MQTT/TLS
- Reading the DHT11 temperature and humidity sensor
- Reading the analog soil moisture sensor
- Determining soil condition
- Publishing sensor data
- Receiving irrigation commands
- Managing automatic irrigation permission
- Controlling the relay
- Publishing the actual pump state
- Publishing the actual automatic irrigation permission
- Updating the LCD
- Controlling the status LED

---

# 2. Hardware

| Component | ESP8266 Pin | GPIO |
|---|---|---:|
| Soil moisture sensor AO | A0 | Analog |
| DHT11 DATA | D4 | GPIO2 |
| Relay IN | D5 | GPIO14 |
| Status LED | D6 | GPIO12 |
| LCD SDA | D2 | GPIO4 |
| LCD SCL | D1 | GPIO5 |

---

# 3. Wiring

## Soil Moisture Sensor

```text
VCC ──> 3.3V
GND ──> GND
AO  ──> A0
```

## DHT11

```text
VCC  ──> 3.3V
GND  ──> GND
DATA ──> D4 / GPIO2
```

## Relay

```text
IN  ──> D5 / GPIO14
GND ──> GND
VCC ──> Appropriate relay supply
```

## LED

```text
D6 / GPIO12 ──> Resistor ──> LED ──> GND
```

## LCD

```text
SDA ──> D2 / GPIO4
SCL ──> D1 / GPIO5
GND ──> GND
VCC ──> Appropriate supply
```

> Do not connect the water pump directly to an ESP8266 GPIO. The GPIO controls the relay; the pump requires a suitable external supply.

---

# 4. Important Firmware Constants

The current firmware uses:

```text
RELAY_PIN       = 14
SENSOR_PIN      = A0
DHT_PIN         = 2
LED_PIN         = 12
MOISTURE_THRESHOLD = 250
DHT_TYPE        = DHT11
```

The LCD is configured as:

```text
Address: 0x27
Size:    16 × 2
```

---

# 5. Libraries

The firmware uses:

```cpp
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
```

Install the corresponding libraries before compiling the firmware.

---

# 6. MQTT Topics

## ESP8266 → Node-RED

```text
college-iot/device01/sensors/temperature
college-iot/device01/sensors/humidity
college-iot/device01/sensors/soil-moisture
college-iot/device01/sensors/soil-condition
college-iot/device01/irrigation/status
college-iot/device01/irrigation/permission
```

## Node-RED → ESP8266

```text
college-iot/device01/actuators/relay
college-iot/device01/irrigation/control
```

---

# 7. MQTT Message Meaning

## Sensor Topics

### Temperature

```text
Topic:
college-iot/device01/sensors/temperature

Direction:
ESP8266 → Node-RED

Payload:
Temperature value
```

### Humidity

```text
Topic:
college-iot/device01/sensors/humidity

Direction:
ESP8266 → Node-RED

Payload:
Humidity value
```

### Soil Moisture

```text
Topic:
college-iot/device01/sensors/soil-moisture

Direction:
ESP8266 → Node-RED

Payload:
Raw analog sensor value
```

### Soil Condition

```text
Topic:
college-iot/device01/sensors/soil-condition

Direction:
ESP8266 → Node-RED

Payload:
Very Dry / Dry / Hydrated / Wet
```

---

# 8. Irrigation Topics

## Automatic Permission Command

```text
Topic:
college-iot/device01/actuators/relay

Direction:
Node-RED → ESP8266

Values:
ENABLE
DISABLE
```

This topic controls whether automatic irrigation is permitted.

It does **not** directly represent the pump state.

---

## Automatic Permission State

```text
Topic:
college-iot/device01/irrigation/permission

Direction:
ESP8266 → Node-RED

Values:
ENABLED
DISABLED
```

This represents the actual automatic irrigation permission state reported by the ESP8266.

---

## Manual Pump Command

```text
Topic:
college-iot/device01/irrigation/control

Direction:
Node-RED → ESP8266

Values:
ON
OFF
```

This is used for manual pump control.

---

## Pump State

```text
Topic:
college-iot/device01/irrigation/status

Direction:
ESP8266 → Node-RED

Values:
ON
OFF
```

This represents the actual pump state.

---

# 9. Sensor Data Flow

```text
DHT11 ───────────────┐
                     │
                     ▼
                 ESP8266
                     │
Soil Sensor ─────────┤
                     │
                     ▼
               Read Sensors
                     │
                     ▼
              Process Readings
                     │
          ┌──────────┼──────────┐
          │          │          │
          ▼          ▼          ▼
     Temperature  Humidity  Soil Moisture
          │          │          │
          └──────────┼──────────┘
                     │
                     ▼
               MQTT Publish
                     │
                     ▼
                HiveMQ Cloud
                     │
                     ▼
                  Node-RED
```

---

# 10. Irrigation Command Flow

## Automatic Irrigation

```text
Node-RED Switch
      │
      ├── ON  → ENABLE
      └── OFF → DISABLE
      │
      ▼
MQTT
college-iot/device01/actuators/relay
      │
      ▼
HiveMQ Cloud
      │
      ▼
ESP8266 MQTT Callback
      │
      ▼
Automatic Permission
      │
      ▼
irrigation/permission
      │
      ▼
Node-RED Switch Feedback
```

## Manual Pump

```text
Node-RED Switch
      │
      ├── ON  → ON
      └── OFF → OFF
      │
      ▼
MQTT
college-iot/device01/irrigation/control
      │
      ▼
HiveMQ Cloud
      │
      ▼
ESP8266 MQTT Callback
      │
      ▼
Relay
      │
      ▼
Pump
      │
      ▼
Actual Pump State
      │
      ▼
irrigation/status
      │
      ▼
Node-RED
```

---

# 11. Automatic Irrigation Logic

The firmware uses the soil moisture threshold:

```text
MOISTURE_THRESHOLD = 250
```

The general decision process is:

```text
Read soil moisture
       │
       ▼
Is automatic irrigation enabled?
       │
   ┌───┴───┐
   │       │
  YES      NO
   │       │
   ▼       ▼
Check     Keep
moisture  automatic
   │       irrigation
   ▼       disabled
Is soil dry?
   │
 ┌─┴─┐
YES  NO
 │    │
 ▼    ▼
Pump  Pump
ON    OFF
```

The exact relay behaviour depends on the firmware's control logic and relay module polarity.

---

# 12. Pump State Feedback

The ESP8266 should always report the actual pump state.

The intended sequence is:

```text
Command received
      │
      ▼
ESP8266 processes command
      │
      ▼
Relay state changes
      │
      ▼
pumpState updated
      │
      ▼
Publish:
college-iot/device01/irrigation/status
      │
      ▼
ON / OFF
```

The status topic should **never** be used to report automatic irrigation permission.

Correct:

```text
irrigation/status
    → ON / OFF
```

Correct:

```text
irrigation/permission
    → ENABLED / DISABLED
```

---

# 13. Important Difference Between Permission and Pump State

These are separate concepts.

### Permission

```text
Automatic irrigation allowed?
```

Possible values:

```text
ENABLED
DISABLED
```

Topic:

```text
college-iot/device01/irrigation/permission
```

### Pump State

```text
Is the pump actually running?
```

Possible values:

```text
ON
OFF
```

Topic:

```text
college-iot/device01/irrigation/status
```

Therefore:

```text
Permission = ENABLED
```

does not necessarily mean:

```text
Pump = ON
```

It only means that the automatic irrigation system is allowed to operate the pump.

---

# 14. Soil Moisture Classification

The soil condition is based on the moisture reading.

Current classification:

```text
0–25%       → Very Dry
25–50%      → Dry
50–75%      → Hydrated
75–100%     → Wet
```

The ESP8266 currently uses:

```text
MOISTURE_THRESHOLD = 250
```

The physical sensor should be calibrated if the project is moved from simulation to a real field.

---

# 15. MQTT Connection

The ESP8266 connects to HiveMQ Cloud using secure MQTT.

```text
ESP8266
   │
   │ Wi-Fi
   ▼
Internet / Network
   │
   │ TLS
   ▼
HiveMQ Cloud
   │
   │ MQTT
   ▼
Node-RED
```

The MQTT connection uses:

```text
Port: 8883
Protocol: MQTT over TLS
```

The username and password must be configured locally and must not be committed to Git.

---

# 16. Startup Sequence

The expected startup sequence is:

```text
ESP8266 Power On
       │
       ▼
Initialize GPIO
       │
       ▼
Initialize DHT11
       │
       ▼
Initialize LCD
       │
       ▼
Connect to Wi-Fi
       │
       ▼
Connect to HiveMQ Cloud
       │
       ▼
Subscribe to command topics
       │
       ├── actuators/relay
       └── irrigation/control
       │
       ▼
Publish initial state
       │
       ├── irrigation/status
       └── irrigation/permission
       │
       ▼
Enter main loop
```

---

# 17. Main Loop

The main loop can be understood as:

```text
Main Loop
    │
    ▼
Maintain MQTT connection
    │
    ▼
Process incoming MQTT messages
    │
    ▼
Read soil moisture
    │
    ▼
Read DHT11
    │
    ▼
Determine soil condition
    │
    ▼
Publish sensor data
    │
    ▼
Run irrigation logic
    │
    ▼
Update relay
    │
    ▼
Update LCD / LED
    │
    ▼
Repeat
```

---

# 18. Troubleshooting

## ESP8266 Does Not Connect to MQTT

Check:

```text
[ ] Wi-Fi connected
[ ] HiveMQ hostname is correct
[ ] Port is 8883
[ ] MQTT username is correct
[ ] MQTT password is correct
[ ] TLS configuration is correct
```

If an error shows a duplicated port such as:

```text
mqtts://example:8883:8883
```

the broker address is incorrectly configured.

---

## Sensor Data Does Not Appear in Node-RED

Check in this order:

```text
DHT11 / Soil Sensor
        ↓
ESP8266 Serial Monitor
        ↓
publishSensorData()
        ↓
MQTT Topic
        ↓
HiveMQ
        ↓
Node-RED MQTT IN
        ↓
Dashboard
```

If the sensor values appear in the Serial Monitor but not in Node-RED, check the MQTT topic and Node-RED subscription.

---

## Pump Turns On After Restart

Check retained MQTT commands.

These command topics should not be retained:

```text
college-iot/device01/actuators/relay
college-iot/device01/irrigation/control
```

Actual state topics can be retained:

```text
college-iot/device01/irrigation/status
college-iot/device01/irrigation/permission
```

If an old command was retained, clear it by publishing an empty retained message to that topic.

---

## Pump Status Shows the Wrong Value

Verify that the ESP8266 publishes:

```text
irrigation/status → ON / OFF
```

and:

```text
irrigation/permission → ENABLED / DISABLED
```

Do not publish `ENABLED` or `DISABLED` to the pump status topic.

---

# 19. Hardware Troubleshooting

## Relay Does Not Switch

Check:

```text
[ ] Relay IN connected to D5 / GPIO14
[ ] Relay has correct supply
[ ] Common GND is present where required
[ ] Relay module is compatible with ESP8266 logic
[ ] Relay active-high/active-low behaviour
```

Some relay modules are active-low. If the relay behaves opposite to what is expected, check the relay module's logic polarity.

---

## Pump Does Not Run

If the relay clicks but the pump does not run:

```text
Relay
  ↓
Check relay contacts
  ↓
Check external pump supply
  ↓
Check pump wiring
  ↓
Check pump itself
```

The ESP8266 should not be used as the pump's power source.

---

## DHT11 Gives Invalid Readings

Check:

```text
[ ] DATA → D4 / GPIO2
[ ] VCC → 3.3V
[ ] GND → GND
[ ] Correct sensor type selected
[ ] Sensor has appropriate pull-up if required
```

The current firmware uses:

```text
DHT11
```

---

# 20. Security

Do not commit:

```text
Wi-Fi password
MQTT password
MQTT credentials
Node-RED credential files
```

The repository should contain the firmware and Node-RED flow structure, but credentials should be configured separately by each user.

---

# 21. Quick Diagnostic Flow

When something fails, use this order.

## Sensor Problem

```text
Is the sensor physically connected?
          │
          ▼
Is ESP8266 reading it?
          │
          ▼
Is ESP8266 publishing it?
          │
          ▼
Is HiveMQ receiving it?
          │
          ▼
Is Node-RED subscribed to the correct topic?
          │
          ▼
Is the Node-RED function processing it?
          │
          ▼
Is the dashboard displaying it?
```

## Pump Problem

```text
Did Node-RED send the command?
          │
          ▼
Is the MQTT topic correct?
          │
          ▼
Did ESP8266 receive the command?
          │
          ▼
Did the relay change state?
          │
          ▼
Did the pump physically run?
          │
          ▼
Did ESP8266 publish the actual state?
          │
          ▼
Did Node-RED receive the state?
          │
          ▼
Did the dashboard update?
```

This approach makes it easier to locate the exact point where communication or hardware control has failed.

---

# 22. Development Notes

The project is designed around a clear separation between:

```text
SENSORS
   ↓
DATA TOPICS
   ↓
MQTT
   ↓
DASHBOARD
```

and:

```text
DASHBOARD
   ↓
COMMAND TOPICS
   ↓
MQTT
   ↓
ESP8266
   ↓
ACTUATOR
   ↓
STATE TOPICS
   ↓
DASHBOARD
```

Keeping these two directions separate makes the system easier to debug and prevents command messages from being confused with actual device state.
