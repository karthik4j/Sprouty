# Sprouty – Precision Agriculture Monitoring System

## 1. Introduction

Sprouty is a precision agriculture monitoring and irrigation system designed to monitor basic field conditions and provide remote control over irrigation.

The system uses an **ESP8266 NodeMCU** as the main controller. It reads soil moisture, temperature, and humidity, determines the current soil condition, and publishes the readings using MQTT. The data is sent through **HiveMQ Cloud** and displayed on a locally hosted **Node-RED Dashboard**.

The system also supports both automatic irrigation permission and manual pump control. The ESP8266 controls a relay connected to the irrigation pump and publishes the actual pump state back to Node-RED.

The overall communication flow is:

```text
                    SENSOR DATA
┌──────────────┐
│    DHT11     │──┐
└──────────────┘  │
                  │
┌──────────────┐  │
│ Soil Sensor  │──┼──> ESP8266 ──MQTT/TLS──> HiveMQ Cloud
└──────────────┘  │                         │
                  │                         │
                  │                         ▼
                  │                   Node-RED
                  │                   Dashboard
                  │                         │
                  │                         │
                  │<──── MQTT Commands ─────┘
                  │
                  ▼
             Relay / Pump
```

---

## 2. Main Features

- Real-time soil moisture monitoring
- Temperature monitoring using DHT11
- Humidity monitoring using DHT11
- Soil condition classification
- MQTT-based communication
- Secure MQTT connection using TLS
- HiveMQ Cloud as the MQTT broker
- Local Node-RED dashboard
- Automatic irrigation permission
- Manual pump control
- Pump state feedback
- Automatic irrigation permission feedback
- 16×2 I2C LCD display
- Status LED
- ESP8266-based control system

---

## 3. System Architecture

The system can be divided into four main parts:

1. **ESP8266 NodeMCU**
2. **HiveMQ Cloud MQTT Broker**
3. **Node-RED Dashboard**
4. **Relay and Water Pump**

### Data Monitoring Path

```text
DHT11
  │
  ├── Temperature
  │
  └── Humidity
       │
       ▼
    ESP8266
       │
       │ MQTT
       ▼
 HiveMQ Cloud
       │
       │ MQTT subscription
       ▼
   Node-RED
       │
       ├── Temperature Gauge
       ├── Temperature Chart
       ├── Humidity Gauge
       ├── Humidity Chart
       ├── Soil Moisture Gauge
       ├── Soil Moisture Chart
       ├── Soil Condition
       └── Pump Status
```

### Irrigation Control Path

```text
Node-RED Dashboard
       │
       ├── Automatic Irrigation Switch
       │
       │       ENABLE / DISABLE
       │
       └── Manual Pump Switch
               │
               │ ON / OFF
               ▼
             MQTT
               │
               ▼
          HiveMQ Cloud
               │
               ▼
            ESP8266
               │
               ▼
             Relay
               │
               ▼
          Water Pump
```

The ESP8266 then publishes the actual state back to MQTT:

```text
ESP8266
   │
   ├── irrigation/status
   │
   └── irrigation/permission
            │
            ▼
        HiveMQ Cloud
            │
            ▼
         Node-RED
            │
            ▼
       Dashboard switches
```

This feedback mechanism is important because the dashboard should represent the **actual state reported by the ESP8266**, rather than simply assuming that the last button press succeeded.

---

# 4. Hardware

## 4.1 Main Components

- ESP8266 NodeMCU
- DHT11 temperature/humidity sensor
- Soil moisture sensor
- Relay module
- Water pump
- 16×2 I2C LCD
- LED
- Resistor for LED
- External power supply for the pump/relay as required

---

# 5. ESP8266 Pinout

The project uses an **ESP8266 NodeMCU**.

| Component | ESP8266 Pin | GPIO |
|---|---|---:|
| Soil moisture sensor AO | A0 | Analog |
| DHT11 DATA | D4 | GPIO2 |
| Relay IN | D5 | GPIO14 |
| Status LED | D6 | GPIO12 |
| LCD SDA | D2 | GPIO4 |
| LCD SCL | D1 | GPIO5 |

## Wiring Summary

### Soil Moisture Sensor

```text
Soil Sensor VCC  ──> 3.3V
Soil Sensor GND  ──> GND
Soil Sensor AO   ──> A0
```

### DHT11

```text
DHT11 VCC   ──> 3.3V
DHT11 GND   ──> GND
DHT11 DATA  ──> D4 / GPIO2
```

### Relay

```text
Relay IN   ──> D5 / GPIO14
Relay GND  ──> GND
Relay VCC  ──> Appropriate supply for the relay module
```

### Status LED

```text
D6 / GPIO12 ──> Resistor ──> LED ──> GND
```

### I2C LCD

```text
LCD SDA ──> D2 / GPIO4
LCD SCL ──> D1 / GPIO5
LCD GND ──> GND
LCD VCC ──> Appropriate supply
```

> **Hardware warning:** The ESP8266 GPIO pins are not 5 V tolerant. Some I2C LCD modules use 5 V pull-up resistors on SDA/SCL. Verify the LCD module's I2C voltage levels before connecting it directly to the ESP8266.

> **Pump warning:** Never power the water pump directly from an ESP8266 GPIO pin. The ESP8266 only controls the relay. The pump must use an appropriate external power supply.

---

# 6. MQTT Communication

MQTT is used as the communication layer between the ESP8266 and Node-RED.

```text
ESP8266
   │
   │ MQTT over TLS
   ▼
HiveMQ Cloud
   │
   │ MQTT
   ▼
Node-RED
```

The MQTT broker is responsible for receiving messages from the ESP8266 and forwarding them to subscribed clients such as Node-RED.

---

# 7. MQTT Topics

The project uses the following MQTT topics.

| Topic | Direction | Purpose |
|---|---|---|
| `college-iot/device01/sensors/temperature` | ESP8266 → Node-RED | Temperature reading |
| `college-iot/device01/sensors/humidity` | ESP8266 → Node-RED | Humidity reading |
| `college-iot/device01/sensors/soil-moisture` | ESP8266 → Node-RED | Raw soil moisture reading |
| `college-iot/device01/sensors/soil-condition` | ESP8266 → Node-RED | Soil condition |
| `college-iot/device01/irrigation/status` | ESP8266 → Node-RED | Actual pump state |
| `college-iot/device01/irrigation/permission` | ESP8266 → Node-RED | Actual automatic irrigation permission |
| `college-iot/device01/actuators/relay` | Node-RED → ESP8266 | Automatic irrigation permission command |
| `college-iot/device01/irrigation/control` | Node-RED → ESP8266 | Manual pump command |

---

# 8. Topic Data Flow

## 8.1 Temperature

```text
DHT11
  │
  │ Temperature
  ▼
ESP8266
  │
  │ Publish
  ▼
college-iot/device01/sensors/temperature
  │
  ▼
HiveMQ Cloud
  │
  │ Node-RED MQTT IN
  ▼
Node-RED
  │
  ├── Temperature Gauge
  ├── Temperature Chart
  └── Debug
```

The ESP8266 reads the temperature from the DHT11 and publishes it to the temperature topic.

---

## 8.2 Humidity

```text
DHT11
  │
  │ Humidity
  ▼
ESP8266
  │
  │ Publish
  ▼
college-iot/device01/sensors/humidity
  │
  ▼
HiveMQ Cloud
  │
  ▼
Node-RED
  │
  ├── Humidity Gauge
  ├── Humidity Chart
  └── Debug
```

---

## 8.3 Soil Moisture

```text
Soil Moisture Sensor
          │
          │ Analog value
          ▼
       ESP8266
          │
          │ Publish raw value
          ▼
college-iot/device01/sensors/soil-moisture
          │
          ▼
      HiveMQ Cloud
          │
          ▼
       Node-RED
          │
          ▼
     Function Node
          │
          │ Convert raw value
          │ to percentage
          ▼
   Soil Moisture Gauge
   Soil Moisture Chart
```

The raw sensor value is converted by Node-RED into a percentage for dashboard display.

Current conversion:

```text
0     → 0%
1024  → 100%
```

The Node-RED conversion is:

```javascript
let moisture = Number(msg.payload);

let percentage = (moisture / 1024) * 100;

percentage = Math.max(0, Math.min(100, percentage));

msg.payload = Number(percentage.toFixed(1));

return msg;
```

> The percentage is a display conversion. A physical soil sensor should be calibrated using actual dry and wet soil conditions rather than assuming that the complete ADC range corresponds perfectly to 0–100% moisture.

---

# 9. Soil Condition

The system also publishes a soil condition classification.

The current classifications are:

| Moisture | Condition |
|---:|---|
| 0–25% | Very Dry |
| 25–50% | Dry |
| 50–75% | Hydrated |
| 75–100% | Wet |

The Node-RED function used for the dashboard classification is:

```javascript
let moisture = Number(msg.payload);

if (moisture <= 25) {
    msg.payload = "Very Dry";
}
else if (moisture <= 50) {
    msg.payload = "Dry";
}
else if (moisture <= 75) {
    msg.payload = "Hydrated";
}
else {
    msg.payload = "Wet";
}

return msg;
```

The flow is:

```text
Soil Sensor
     │
     ▼
  ESP8266
     │
     ▼
soil-moisture
     │
     ▼
Node-RED
     │
     ▼
Convert raw value → percentage
     │
     ▼
Classify moisture
     │
     ▼
Soil Condition
```

---

# 10. Automatic Irrigation

Automatic irrigation is controlled using an **irrigation permission** state.

The dashboard switch does not directly mean that the pump is ON.

Instead:

```text
Automatic Irrigation
        │
        ▼
Permission to allow automatic irrigation
```

The command topic is:

```text
college-iot/device01/actuators/relay
```

The possible command values are:

```text
ENABLE
DISABLE
```

## Enable Flow

```text
Dashboard
Automatic Irrigation = ON
        │
        ▼
Node-RED Function
        │
        │ "ENABLE"
        ▼
MQTT OUT
        │
        ▼
college-iot/device01/actuators/relay
        │
        ▼
HiveMQ Cloud
        │
        ▼
ESP8266
        │
        ▼
Automatic irrigation permission = ENABLED
```

## Disable Flow

```text
Dashboard
Automatic Irrigation = OFF
        │
        ▼
Node-RED Function
        │
        │ "DISABLE"
        ▼
MQTT OUT
        │
        ▼
college-iot/device01/actuators/relay
        │
        ▼
HiveMQ Cloud
        │
        ▼
ESP8266
        │
        ▼
Automatic irrigation permission = DISABLED
```

Node-RED converts the dashboard boolean into the MQTT command:

```javascript
msg.payload = msg.payload ? "ENABLE" : "DISABLE";
msg.topic = "college-iot/device01/actuators/relay";

return msg;
```

---

# 11. Automatic Irrigation Feedback

The ESP8266 publishes its actual permission state to:

```text
college-iot/device01/irrigation/permission
```

Possible values:

```text
ENABLED
DISABLED
```

The feedback flow is:

```text
ESP8266
   │
   │ ENABLED / DISABLED
   ▼
college-iot/device01/irrigation/permission
   │
   ▼
HiveMQ Cloud
   │
   ▼
Node-RED MQTT IN
   │
   ▼
Function
   │
   ├── ENABLED  → true
   └── DISABLED → false
   │
   ▼
Automatic Irrigation Switch
```

Node-RED function:

```javascript
msg.payload = msg.payload === "ENABLED";

return msg;
```

This means the dashboard switch is updated from the state reported by the ESP8266.

---

# 12. Manual Pump Control

Manual pump control uses a separate MQTT topic:

```text
college-iot/device01/irrigation/control
```

The command values are:

```text
ON
OFF
```

## Manual Pump ON

```text
Dashboard
Start Pump = ON
        │
        ▼
Node-RED Function
        │
        │ "ON"
        ▼
MQTT OUT
        │
        ▼
college-iot/device01/irrigation/control
        │
        ▼
HiveMQ Cloud
        │
        ▼
ESP8266
        │
        ▼
Relay
        │
        ▼
Water Pump
```

Node-RED function:

```javascript
msg.payload = msg.payload ? "ON" : "OFF";
msg.topic = "college-iot/device01/irrigation/control";

return msg;
```

---

# 13. Pump Status Feedback

The ESP8266 publishes the actual pump state to:

```text
college-iot/device01/irrigation/status
```

Possible values:

```text
ON
OFF
```

The flow is:

```text
ESP8266
   │
   │ Actual pump state
   ▼
college-iot/device01/irrigation/status
   │
   ▼
HiveMQ Cloud
   │
   ▼
Node-RED MQTT IN
   │
   ▼
Function
   │
   ├── ON  → true
   └── OFF → false
   │
   ▼
Start Pump Switch
```

Node-RED function:

```javascript
msg.payload = msg.payload === "ON";

return msg;
```

The pump status should represent the **actual state of the pump**, not the command that was sent.

---

# 14. Important Difference Between Commands and States

There are two different types of MQTT topics in this project.

## Command Topics

These tell the ESP8266 what to do.

```text
college-iot/device01/actuators/relay
college-iot/device01/irrigation/control
```

Commands:

```text
ENABLE
DISABLE

ON
OFF
```

## State Topics

These tell Node-RED what the ESP8266 is actually doing.

```text
college-iot/device01/irrigation/permission
college-iot/device01/irrigation/status
```

States:

```text
ENABLED
DISABLED

ON
OFF
```

The distinction is important:

```text
COMMAND
Node-RED ──────────────> ESP8266

STATE
Node-RED <────────────── ESP8266
```

This prevents the dashboard from becoming out of sync with the physical device.

---

# 15. Complete Irrigation Logic

The complete logic can be understood as:

```text
                    Soil Moisture
                         │
                         ▼
                     ESP8266
                         │
                         ▼
                 Is automatic
                 irrigation enabled?
                    /          \
                  YES           NO
                   │             │
                   ▼             ▼
             Check moisture    Do not start
                   │             │
             Soil is dry?        │
              /       \          │
            YES        NO        │
             │          │        │
             ▼          ▼        │
        Start Pump   Keep Pump   │
             │          OFF      │
             └──────┬─────┘      │
                    ▼            │
              Publish actual     │
              pump status        │
                    │            │
                    └──────┬─────┘
                           ▼
                     MQTT Status
                           │
                           ▼
                       Node-RED
```

Manual control provides another route:

```text
Dashboard
    │
    │ Manual ON/OFF
    ▼
MQTT Command
    │
    ▼
ESP8266
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
MQTT Status
    │
    ▼
Node-RED Dashboard
```

---

# 16. Sensor Publishing Flow

The ESP8266 periodically reads the sensors.

The general firmware flow is:

```text
ESP8266 starts
      │
      ▼
Connect to Wi-Fi
      │
      ▼
Connect to HiveMQ Cloud
      │
      ▼
Subscribe to control topics
      │
      ▼
Read soil moisture
      │
      ├───────────────┐
      ▼               ▼
Read DHT11        Determine soil
temperature       condition
and humidity          │
      │               │
      └───────┬───────┘
              ▼
       Publish sensor data
              │
              ▼
          HiveMQ Cloud
              │
              ▼
          Node-RED
              │
              ▼
           Dashboard
```

The sensor topics are:

```text
temperature
humidity
soil-moisture
soil-condition
```

---

# 17. Node-RED Flow

The main Node-RED processing can be represented as:

```text
                    HiveMQ Cloud
                         │
             ┌───────────┼───────────┐
             │           │           │
             ▼           ▼           ▼
        Temperature   Humidity   Soil Moisture
             │           │           │
             ▼           ▼           ▼
          Function    Function    Function
             │           │           │
             ▼           ▼           ▼
          Gauge/      Gauge/      Gauge/
          Chart       Chart       Chart
                                     │
                                     ▼
                                Condition
                                     │
                                     ▼
                               Dashboard Text


                    HiveMQ Cloud
                         │
             ┌───────────┴───────────┐
             │                       │
             ▼                       ▼
        Pump Status             Permission
             │                       │
             ▼                       ▼
       Function Node            Function Node
             │                       │
             ▼                       ▼
       Pump Switch             Auto Switch
```

Control flows go in the opposite direction:

```text
Automatic Switch
       │
       ▼
Function
       │
       ▼
MQTT OUT
       │
       ▼
actuators/relay
       │
       ▼
ESP8266


Manual Pump Switch
       │
       ▼
Function
       │
       ▼
MQTT OUT
       │
       ▼
irrigation/control
       │
       ▼
ESP8266
```

---

# 18. MQTT Message Reference

## Sensor Messages

### Temperature

```text
Topic:
college-iot/device01/sensors/temperature

Payload:
Temperature value
```

### Humidity

```text
Topic:
college-iot/device01/sensors/humidity

Payload:
Humidity value
```

### Soil Moisture

```text
Topic:
college-iot/device01/sensors/soil-moisture

Payload:
Raw analog soil moisture value
```

### Soil Condition

```text
Topic:
college-iot/device01/sensors/soil-condition

Payload:
Very Dry
Dry
Hydrated
Wet
```

## Irrigation Messages

### Automatic Permission Command

```text
Topic:
college-iot/device01/actuators/relay

Payload:
ENABLE
DISABLE
```

### Automatic Permission State

```text
Topic:
college-iot/device01/irrigation/permission

Payload:
ENABLED
DISABLED
```

### Manual Pump Command

```text
Topic:
college-iot/device01/irrigation/control

Payload:
ON
OFF
```

### Pump State

```text
Topic:
college-iot/device01/irrigation/status

Payload:
ON
OFF
```

---

# 19. Retained MQTT Messages

The project uses retained messages for **actual device states** so that Node-RED can recover the latest known state after reconnecting.

Recommended retained topics:

```text
college-iot/device01/irrigation/status
college-iot/device01/irrigation/permission
```

Command topics should **not** be retained:

```text
college-iot/device01/actuators/relay
college-iot/device01/irrigation/control
```

This distinction prevents an old command from being replayed to the ESP8266 after it restarts.

For example, if:

```text
irrigation/control = ON
```

were retained, a newly connected ESP8266 could receive that old `ON` command immediately after connecting.

That could cause the pump to start unexpectedly.

Therefore:

```text
COMMANDS
Retain = false

ACTUAL STATES
Retain = true
```

---

# 20. Clearing an Existing Retained Command

If a command topic was previously configured as retained, the retained message must be cleared.

The retained message can be removed by publishing an **empty payload with Retain enabled** to the same topic.

For example:

```text
Topic:
college-iot/device01/irrigation/control

Payload:
<empty>

Retain:
true
```

Do this once when necessary.

After clearing it, normal command messages should use:

```text
Retain = false
```

---

# 21. Troubleshooting

## 21.1 Dashboard Sensor Values Are Empty

Check the ESP8266 serial monitor first.

The ESP8266 must actually call the sensor publishing function.

The expected flow is:

```text
Sensor read
    │
    ▼
publishSensorData()
    │
    ├── temperature
    ├── humidity
    ├── soil-moisture
    └── soil-condition
```

If the sensors are being read but nothing is appearing in Node-RED:

1. Check Wi-Fi connection.
2. Check MQTT connection.
3. Check the MQTT topic names.
4. Check that Node-RED MQTT IN nodes are subscribed to the exact topics.
5. Check the MQTT debug nodes.
6. Check that `publishSensorData()` is being called.

---

## 21.2 Pump Turns ON After ESP8266 Restart

The first thing to check is whether the command topic is retained.

Check:

```text
college-iot/device01/irrigation/control
college-iot/device01/actuators/relay
```

These should not have retained commands.

Clear any old retained command and set the MQTT OUT nodes to:

```text
Retain = false
```

Actual state topics may remain retained.

---

## 21.3 Node-RED Shows "Invalid Payload Value"

Check the payload received by the switch.

For the pump status switch, valid values before conversion are:

```text
ON
OFF
```

For the permission switch:

```text
ENABLED
DISABLED
```

For example, this is incorrect:

```text
irrigation/status
    ↓
"ENABLED"
```

The pump status topic should contain:

```text
"ON"
```

or:

```text
"OFF"
```

The permission topic should contain:

```text
"ENABLED"
```

or:

```text
"DISABLED"
```

The two topics should not be mixed.

---

## 21.4 Dashboard Switch Changes but ESP8266 Does Not Respond

Trace the command in this order:

```text
Dashboard Switch
      ↓
Node-RED Function
      ↓
MQTT OUT
      ↓
HiveMQ Cloud
      ↓
ESP8266 MQTT callback
      ↓
Relay/Pump
```

Check:

- Function node output
- MQTT OUT topic
- MQTT broker connection
- ESP8266 MQTT connection
- ESP8266 subscription
- MQTT callback
- Relay wiring
- Relay active-high/active-low behaviour

---

## 21.5 Pump Status Does Not Match the Switch

Do not troubleshoot only the dashboard switch.

The intended flow is:

```text
Dashboard command
       ↓
ESP8266
       ↓
Physical relay
       ↓
Actual pump state
       ↓
irrigation/status
       ↓
Dashboard switch
```

The dashboard should ultimately follow:

```text
irrigation/status
```

not merely the command that was sent.

---

## 21.6 MQTT Connection Fails

Check:

```text
Broker hostname
Port: 8883
Username
Password
TLS configuration
Wi-Fi connection
```

The ESP8266 uses a secure MQTT connection.

Node-RED must also use the correct HiveMQ Cloud connection settings.

If an MQTT URL reports an error similar to:

```text
Invalid port in URL
```

check that the broker address does not contain the port twice.

Incorrect:

```text
mqtts://broker-address:8883:8883
```

Correct:

```text
mqtts://broker-address:8883
```

---

# 22. Important Debugging Strategy

When diagnosing the system, follow the data path rather than changing several components at once.

For sensor problems:

```text
Sensor
  ↓
ESP8266 Serial Monitor
  ↓
MQTT Publish
  ↓
HiveMQ
  ↓
Node-RED MQTT IN
  ↓
Function
  ↓
Dashboard
```

For control problems:

```text
Dashboard
  ↓
Node-RED Function
  ↓
MQTT OUT
  ↓
HiveMQ
  ↓
ESP8266 MQTT callback
  ↓
Relay
  ↓
Pump
  ↓
ESP8266 state
  ↓
MQTT status
  ↓
Node-RED
  ↓
Dashboard
```

This makes it possible to identify which layer is actually failing.

---

# 23. Node-RED Dashboard

Node-RED is run locally on the computer used for the demonstration.

The dashboard is available at:

```text
http://localhost:1880/dashboard
```

The dashboard displays:

- Temperature
- Humidity
- Soil moisture
- Soil condition
- Pump status
- Automatic irrigation permission
- Manual pump control

---

# 24. Software Requirements

The project requires:

- Arduino-compatible ESP8266 development environment
- ESP8266 board support
- Node-RED
- FlowFuse Dashboard 2.0
- MQTT support for Node-RED
- HiveMQ Cloud account
- Git, if using the repository workflow

### ESP8266 Libraries

The firmware uses:

```text
ESP8266WiFi
WiFiClientSecure
PubSubClient
DHT
LiquidCrystal_I2C
```

---

# 25. Running the Project

## ESP8266

1. Open the Arduino firmware in the Arduino IDE or compatible development environment.
2. Select the appropriate ESP8266 NodeMCU board.
3. Configure the Wi-Fi credentials.
4. Configure the HiveMQ MQTT connection.
5. Upload the firmware.
6. Open the Serial Monitor.
7. Confirm that the ESP8266 connects to Wi-Fi.
8. Confirm that it connects to HiveMQ Cloud.
9. Confirm that sensor readings are being published.

## Node-RED

1. Install Node-RED.
2. Install the required dashboard and MQTT nodes.
3. Open the Sprouty Node-RED project.
4. Configure the MQTT broker credentials locally.
5. Deploy the flow.
6. Open:

```text
http://localhost:1880/dashboard
```

7. Check the sensor values.
8. Test the automatic irrigation permission switch.
9. Test the manual pump switch.
10. Confirm that the dashboard reflects the actual state reported by the ESP8266.

---

# 26. Project Structure

The repository is organized approximately as follows:

```text
Sprouty/
│
├── .gitignore
├── README.md
├── package.json
├── flows.json
│
└── Arduino/
    ├── Sprouty.ino
    └── README.md
```

Node-RED credentials should remain local and must not be committed to Git.

---

# 27. Security

The project uses MQTT over TLS through HiveMQ Cloud.

Credentials such as:

- MQTT username
- MQTT password
- Wi-Fi password

must not be committed to the repository.

The Node-RED credentials file is intentionally excluded from Git.

Each teammate should configure their own local credentials when setting up the project.

Before pushing changes to GitHub, check:

```text
git status
```

and make sure credential files are not being staged.

---

# 28. Configuration Checklist for a New Teammate

```text
[ ] Install Node-RED
[ ] Install required Node-RED dashboard/MQTT nodes
[ ] Clone the Sprouty repository
[ ] Configure Node-RED MQTT credentials locally
[ ] Install ESP8266 board support
[ ] Install required Arduino libraries
[ ] Configure Wi-Fi credentials
[ ] Configure MQTT credentials
[ ] Verify HiveMQ broker settings
[ ] Upload ESP8266 firmware
[ ] Open Serial Monitor
[ ] Confirm MQTT connection
[ ] Start Node-RED
[ ] Deploy the flow
[ ] Open /dashboard
[ ] Confirm sensor data
[ ] Test automatic irrigation
[ ] Test manual pump control
[ ] Confirm state feedback
```

---

# 29. Known Project Details

| Parameter | Value |
|---|---|
| Microcontroller | ESP8266 NodeMCU |
| Temperature/Humidity Sensor | DHT11 |
| Soil Sensor | Analog soil moisture sensor |
| MQTT Broker | HiveMQ Cloud |
| MQTT Port | 8883 |
| MQTT Security | TLS |
| Dashboard | Node-RED + FlowFuse Dashboard 2.0 |
| Dashboard Location | Localhost |
| Soil Moisture Threshold | 250 |
| LCD | 16×2 I2C |
| LCD Address | `0x27` |

The current firmware uses:

```text
MOISTURE_THRESHOLD = 250
```

The threshold can be adjusted after calibrating the physical soil sensor.

---

# 30. Final System Overview

The complete Sprouty system can be summarized as:

```text
                         ┌──────────────────┐
                         │      DHT11       │
                         │ Temp + Humidity  │
                         └────────┬─────────┘
                                  │
                         ┌────────▼─────────┐
                         │  Soil Moisture   │
                         │     Sensor       │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │     ESP8266      │
                         │                  │
                         │ Sensor Reading   │
                         │ Irrigation Logic │
                         │ MQTT Client      │
                         └────────┬─────────┘
                                  │
                     MQTT over TLS│
                                  ▼
                         ┌──────────────────┐
                         │  HiveMQ Cloud    │
                         │   MQTT Broker    │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │     Node-RED     │
                         │                  │
                         │ Data Processing  │
                         │ Dashboard        │
                         │ Control          │
                         └────────┬─────────┘
                                  │
                     MQTT Commands│
                                  ▼
                         ┌──────────────────┐
                         │     ESP8266      │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │      Relay       │
                         └────────┬─────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │    Water Pump    │
                         └──────────────────┘
```

The key principle of the system is:

```text
Sensors → ESP8266 → MQTT → Node-RED
                         ↓
                    Dashboard
                         ↓
                   MQTT Commands
                         ↓
                      ESP8266
                         ↓
                    Relay / Pump
                         ↓
                  Actual State
                         ↓
                    MQTT Feedback
                         ↓
                     Dashboard
```

This structure allows each part of the system to be diagnosed independently while keeping the communication between the ESP8266, HiveMQ Cloud, Node-RED, and irrigation hardware clear.
