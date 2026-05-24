<<<<<<< HEAD
# SOME/IP RPC Communication — Window ECU & Climate ECU

A complete SOME/IP-based Remote Procedure Call (RPC) communication system between two simulated Electronic Control Units (ECUs), implemented in C++ using the vSomeIP middleware library on Raspberry Pi.

---

## Project Overview

This project simulates an automotive ECU network where:

- **Window ECU** acts as the SOME/IP **service provider (server)**
- **Climate ECU** acts as the SOME/IP **client (consumer)**

The Climate ECU sends an RPC request asking for the current window position. The Window ECU processes the request and returns the window position value as a response.

---

## Communication Flow

```
Climate ECU
    |
    |  RPC Request: "What is the current window position?"
    v
Window ECU
    |
    |  RPC Response: 30%
    v
Climate ECU
    |
    |  Prints: "Window position received: 30%"
```

---

## SOME/IP Service Identifiers

| Identifier  | Value  |
|-------------|--------|
| Service ID  | 0x1111 |
| Instance ID | 0x2222 |
| Method ID   | 0x3333 |

---

## Project Structure

```
someip-rpc-project/
├── window_ecu.hpp          # Window ECU service class (server logic)
├── window_ecu_main.cpp     # Window ECU entry point
├── climate_ecu.hpp         # Climate ECU client class (client logic)
├── climate_ecu_main.cpp    # Climate ECU entry point
├── CMakeLists.txt          # CMake build configuration
├── helloworld-local.json   # vSomeIP configuration file
└── README.md
```

---

## Hardware & Environment

- **Hardware:** Raspberry Pi
- **OS:** Raspberry Pi OS (Debian Linux)
- **Middleware:** vSomeIP 3.7.2
- **Language:** C++
- **Build System:** CMake
- **Access:** SSH from Windows PC

---

## Dependencies

- [vSomeIP 3.7.2](https://github.com/COVESA/vsomeip)
- CMake (>= 3.10)
- GCC/G++
- Boost libraries (required by vSomeIP)

---

## Installing vSomeIP

```bash
git clone https://github.com/COVESA/vsomeip.git
cd vsomeip
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

---

## Building the Project

```bash
git clone https://github.com/YOURUSERNAME/someip-rpc-project.git
cd someip-rpc-project
mkdir build && cd build
cmake ..
make
```

---

## Running the System

Open two terminals on the Raspberry Pi.

**Terminal 1 — Start the Window ECU (Server)**
```bash
cd build
VSOMEIP_CONFIGURATION=../helloworld-local.json \
VSOMEIP_APPLICATION_NAME=hello_world_service \
./window_ecu
```

**Terminal 2 — Start the Climate ECU (Client)**
```bash
cd build
VSOMEIP_CONFIGURATION=../helloworld-local.json \
VSOMEIP_APPLICATION_NAME=hello_world_client \
./climate_ecu
```

**Expected output on Climate ECU terminal:**
```
Window position received: 30 %
```

---

## Key Concepts Demonstrated

- **SOME/IP middleware communication** using vSomeIP
- **RPC request/response** pattern between ECUs
- **Service Discovery (SD)** — client automatically detects when the service becomes available
- **Payload serialization/deserialization** — window position transmitted as a byte value
- **Service-oriented automotive architecture** principles
- **Distributed ECU communication** on embedded Linux

---

## RPC vs Pub/Sub

This project uses **RPC (Remote Procedure Call)**:
- The client explicitly requests data and waits for a response
- Communication is on-demand

This differs from **Pub/Sub**:
- The server continuously broadcasts data regardless of whether anyone asked

---

## Planned Future Extensions

- **Speed ECU** as a second SOME/IP client
- Dynamic window position values (instead of hardcoded 30%)
- Multiple RPC methods (e.g. open/close window commands)
- Multi-client communication with the same Window ECU service
- Structured payloads with multiple data fields

### Future Architecture

```
Climate ECU ──┐
              ├──► Window ECU (Service Provider)
Speed ECU  ──┘
```

---

## Relevance to Automotive Software

This project demonstrates concepts used in:

- **AUTOSAR Adaptive** middleware
- **Service-oriented vehicle architectures**
- **Zonal ECU architectures**
- **Distributed embedded systems**
- **Automotive middleware frameworks**
=======
# Speed ECU RPC Test

Small test for SOME/IP RPC using Python and someipy.

The idea is to have a Speed ECU that provides a simple method:

GetVehicleSpeed()

The Climate ECU calls this method and receives one speed value.

## Files

- speed_ecu_provider.py
- climate_ecu_client.py
- someipyd.json

## IDs used for the test

- Service ID: 0x1234
- Instance ID: 0x5678
- Method ID: 0x0421

## Payload

For now the response is only one byte.

Example:

60 km/h

## Current status

The local test in WSL is running.

The provider receives the request and sends back the speed value.

Current output:

Vehicle speed received: 60 km/h
>>>>>>> speed_ecu/main
