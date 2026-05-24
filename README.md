# SOME/IP RPC Multi-ECU Communication System
### Built on Raspberry Pi using vSomeIP (C++) and someipy (Python)

**Project Members:**
- Bashar Hejazi
- Jorge Castellanos
- Mohamed Eltahan

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Hardware & Environment](#3-hardware--environment)
4. [Phase 1 — Setting Up the Raspberry Pi](#4-phase-1--setting-up-the-raspberry-pi)
5. [Phase 2 — Installing vSomeIP](#5-phase-2--installing-vsomeip)
6. [Phase 3 — Hello World Benchmark](#6-phase-3--hello-world-benchmark)
7. [Phase 4 — Custom ECU Development (C++)](#7-phase-4--custom-ecu-development-c)
8. [Phase 5 — Python Speed ECU Integration](#8-phase-5--python-speed-ecu-integration)
9. [Phase 6 — GitHub Repository Setup](#9-phase-6--github-repository-setup)
10. [Running the Full System (5 Terminals)](#10-running-the-full-system-5-terminals)
11. [Debugging Journey & Issues Faced](#11-debugging-journey--issues-faced)
12. [Final Outputs](#12-final-outputs)
13. [Key Concepts Demonstrated](#13-key-concepts-demonstrated)
14. [Future Improvements](#14-future-improvements)

---

## 1. Project Overview

This project implements a complete **SOME/IP-based Remote Procedure Call (RPC) communication system** between three simulated Electronic Control Units (ECUs) running on a Raspberry Pi.

The project started from the official vSomeIP **Hello World** example, which we used as a benchmark and reference point. From there, we adapted and extended it into a realistic automotive ECU communication prototype with a Window ECU (C++), a Climate ECU (C++), and a Speed ECU (Python) — all communicating over SOME/IP simultaneously.

The end result is a working multi-middleware integration where **vSomeIP (C++)** and **someipy (Python)** run side by side on the same machine, each serving and consuming SOME/IP RPC services independently.

---

## 2. System Architecture

### Final Architecture

```
┌──────────────────────────────────────────────────────┐
│                    Raspberry Pi                      │
│                                                      │
│   ┌───────────────┐         ┌───────────────────┐   │
│   │  Climate ECU  │──RPC───►│    Window ECU     │   │
│   │  (C++/vSomeIP)│◄──30%───│  (C++/vSomeIP)   │   │
│   └───────────────┘         └───────────────────┘   │
│                                                      │
│   ┌───────────────┐         ┌───────────────────┐   │
│   │  Speed Client │──RPC───►│    Speed ECU      │   │
│   │  (Python)     │◄─60km/h─│  (Python/someipy) │   │
│   └───────────────┘         └───────────────────┘   │
│                                                      │
│   ┌──────────────────────────────────────────────┐  │
│   │         someipy Daemon (someipyd)             │  │
│   │  Manages Python SOME/IP SD & message routing │  │
│   └──────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

### ECU Roles

| ECU | Role | Language | Middleware | Returns |
|---|---|---|---|---|
| Window ECU | SOME/IP Server | C++ | vSomeIP 3.7.2 | Window position: 30% |
| Climate ECU | SOME/IP Client | C++ | vSomeIP 3.7.2 | Requests window position |
| Speed ECU | SOME/IP Server | Python | someipy 2.1.2 | Vehicle speed: 60 km/h |
| Speed Client | SOME/IP Client | Python | someipy 2.1.2 | Requests vehicle speed |

### SOME/IP Service Identifiers

**Window ECU (C++ / vSomeIP)**
| Identifier | Value |
|---|---|
| Service ID | 0x1111 |
| Instance ID | 0x2222 |
| Method ID | 0x3333 |

**Speed ECU (Python / someipy)**
| Identifier | Value |
|---|---|
| Service ID | 0x1234 |
| Instance ID | 0x5678 |
| Method ID | 0x0421 |

---

## 3. Hardware & Environment

- **Hardware:** Raspberry Pi
- **OS:** Raspberry Pi OS (Debian Linux)
- **Network:** Ethernet (`eth0`), dynamically assigned IP
- **Remote Access:** SSH from Windows PC
- **C++ Middleware:** vSomeIP 3.7.2
- **Python Middleware:** someipy 2.1.2
- **Build System:** CMake 3.31.6
- **Compiler:** GCC/G++
- **Python Version:** 3.13

---

## 4. Phase 1 — Setting Up the Raspberry Pi

The Raspberry Pi was accessed remotely from a Windows PC using SSH:

```bash
ssh pi@raspberrypi.local
```

After entering the password, the terminal connected successfully to the Linux system. All subsequent development was done entirely through this SSH connection.

---

## 5. Phase 2 — Installing vSomeIP

### Cloning the Repository

```bash
git clone https://github.com/COVESA/vsomeip.git
```

### Building and Installing

```bash
cd vsomeip
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

The installation included:

- `libvsomeip3.so`
- `libvsomeip3-cfg.so`
- `libvsomeip3-sd.so`
- `libvsomeip3-e2e.so`
- Header files and CMake package configuration

---

## 6. Phase 3 — Hello World Benchmark

Before writing any custom code, we verified the vSomeIP installation using the bundled Hello World example. This served as our **benchmark** — a reference point to confirm that SOME/IP communication, service discovery, and request/response flow all worked correctly on the Pi.

### Finding the Example

Running `./hello_world_service` from the wrong directory gave:
```
No such file or directory
```

The correct location was found using:
```bash
find ~/vsomeip/build -name "hello_world*"
# Result: /home/pi/vsomeip/build/examples/hello_world/
```

### Running the Benchmark

**Terminal 1:**
```bash
cd ~/vsomeip/build/examples/hello_world
./hello_world_service
```

**Terminal 2:**
```bash
cd ~/vsomeip/build/examples/hello_world
./hello_world_client
```

**Output:**
```
Received: Hello World
```

This confirmed that vSomeIP was correctly installed and that SOME/IP RPC communication was working end to end.

### What the Hello World Example Does

The Hello World example is a simple SOME/IP service where:
- The **service** registers a method handler and waits for requests
- The **client** discovers the service, sends the string `"World"` as payload
- The **service** responds with `"Hello World"`

The identifiers used in the original example were:

| Identifier | Value |
|---|---|
| Service ID | 0x1111 |
| Instance ID | 0x2222 |
| Method ID | 0x3333 |

We kept these same identifiers in our custom Window/Climate ECU implementation.

---

## 7. Phase 4 — Custom ECU Development (C++)

### Project Setup

We created a new project directory and copied the Hello World example as a starting point:

```bash
mkdir ~/someip_rpc_project
cp -r ~/vsomeip/examples/hello_world ./reference_hello_world
```

The reference copy was kept untouched so we could always go back to the original working baseline.

### Renaming and Adapting

The Hello World files were adapted into our automotive ECU concept:

| Original | Renamed To |
|---|---|
| `hello_world_service.hpp` | `window_ecu.hpp` |
| `hello_world_service_main.cpp` | `window_ecu_main.cpp` |
| `hello_world_client.hpp` | `climate_ecu.hpp` |
| `hello_world_client_main.cpp` | `climate_ecu_main.cpp` |

### What We Changed

The core logic was adapted from the generic Hello World into a realistic ECU scenario:

**Window ECU (Server)** — instead of echoing a string, the server now:
- Waits for an incoming RPC request
- Creates a response with `uint8_t window_position = 30`
- Pushes that byte value into a payload vector
- Sends it back to the client

```cpp
uint8_t window_position = 30;
std::vector<vsomeip::byte_t> pl_data;
pl_data.push_back(window_position);
std::shared_ptr<vsomeip::payload> resp_pl = rtm_->create_payload();
resp_pl->set_data(pl_data);
resp->set_payload(resp_pl);
app_->send(resp);
```

**Climate ECU (Client)** — instead of sending a "World" string and printing whatever comes back, the client now:
- Discovers the Window ECU service via SOME/IP Service Discovery
- Sends an RPC request when the service becomes available
- Receives the response payload
- Extracts the first byte as the window position value
- Prints it as a percentage

```cpp
uint8_t window_position = pl->get_data()[0];
LOG_INF("Window position received: %d %%", static_cast<int>(window_position));
```

### Building the Project

```bash
mkdir ~/someip_rpc_project/window_rpc/build
cd ~/someip_rpc_project/window_rpc/build
cmake ..
make
```

### Running the C++ ECUs

**Terminal 1 — Window ECU:**
```bash
cd ~/someip_rpc_project/window_rpc/build
VSOMEIP_CONFIGURATION=../helloworld-local.json \
VSOMEIP_APPLICATION_NAME=hello_world_service \
./window_ecu
```

**Terminal 2 — Climate ECU:**
```bash
cd ~/someip_rpc_project/window_rpc/build
VSOMEIP_CONFIGURATION=../helloworld-local.json \
VSOMEIP_APPLICATION_NAME=hello_world_client \
./climate_ecu
```

**Output on Climate ECU terminal:**
```
Window position received: 30 %
```

This marked the completion of Phase 4 — a fully working automotive-style RPC system built on top of the Hello World benchmark.

---

## 8. Phase 5 — Python Speed ECU Integration

With the C++ system working, the next goal was to integrate a **Python-based Speed ECU** written using the `someipy` library — a different SOME/IP implementation from vSomeIP. This is the most challenging and interesting part of the project, as it required two different middleware stacks to coexist on the same machine.

### Installing someipy

```bash
pip install someipy --break-system-packages
```

> `--break-system-packages` is required on Raspberry Pi OS due to PEP 668 restrictions on system-managed Python environments.

### The someipy Daemon

Unlike vSomeIP which handles everything internally, someipy requires a **separate daemon process** (`someipyd`) to manage all SOME/IP network communication and service discovery. All someipy applications connect to this daemon via Unix Domain Sockets.

Starting the daemon:
```bash
someipyd --config ~/someip_rpc_project/window_rpc/someipyd.json
```

The daemon config (`someipyd.json`):
```json
{
  "interface": "100.88.162.130",
  "sd_address": "224.224.224.245",
  "sd_port": 30490
}
```

### Speed ECU Server

The Speed ECU registers a `GetVehicleSpeed()` method handler that returns `60 km/h` as a byte payload:

```python
async def get_vehicle_speed_handler(payload: bytes, addr: Tuple[str, int]) -> MethodResult:
    vehicle_speed_kmh = 60
    result = MethodResult()
    result.message_type = MessageType.RESPONSE
    result.return_code = ReturnCode.E_OK
    result.payload = bytes([vehicle_speed_kmh])
    return result
```

### Speed Client

To trigger the Speed ECU and see the response, a Python client was written that:
1. Connects to the someipy daemon
2. Waits 3 seconds for service discovery to complete
3. Calls `GetVehicleSpeed()` on the Speed ECU
4. Prints the received speed value

**Output:**
```
Connecting to someipy daemon...
Connected! Waiting for Speed ECU...
Calling GetVehicleSpeed()...
Vehicle speed received: 60 km/h
```

---

## 9. Phase 6 — GitHub Repository Setup

The project was version-controlled and pushed to GitHub directly from the Raspberry Pi.

### Repository Initialization

```bash
cd ~/someip_rpc_project/window_rpc
git init
git add window_ecu.hpp window_ecu_main.cpp climate_ecu.hpp climate_ecu_main.cpp CMakeLists.txt helloworld-local.json
git commit -m "Initial commit - SOME/IP RPC Window and Climate ECU"
git branch -M main
git remote add origin https://github.com/Beeshh/someip-rpc-project.git
git push -u origin main
```

### Merging the Speed ECU Repository

The Speed ECU code existed in a separate repository. It was merged into the main repo:

```bash
git remote add speed_ecu https://github.com/jlcasteoca-cmyk/someip-speed-rpc.git
git fetch speed_ecu
git merge speed_ecu/main --allow-unrelated-histories
git push origin main
```

This caused a README conflict which was resolved manually (see Debugging section).

---

## 10. Running the Full System (5 Terminals)

The complete integrated system requires **5 SSH terminals** open simultaneously on the Raspberry Pi.

### Startup Order (important — follow this sequence)

**Terminal 1 — someipy Daemon (start first)**
```bash
ssh pi@raspberrypi.local
someipyd --config ~/someip_rpc_project/window_rpc/someipyd.json
```
```
someipyd [INFO]: Unix domain socket server started at /tmp/someipyd.sock
```

**Terminal 2 — Speed ECU Server**
```bash
ssh pi@raspberrypi.local
python3 ~/someip_rpc_project/window_rpc/speed_ecu.py
```
```
Speed ECU provider started
Offering GetVehicleSpeed() RPC service...
```

**Terminal 3 — Window ECU Server**
```bash
ssh pi@raspberrypi.local
cd ~/someip_rpc_project/window_rpc/build
VSOMEIP_CONFIGURATION=../helloworld-local.json VSOMEIP_APPLICATION_NAME=hello_world_service ./window_ecu
```

**Terminal 4 — Climate ECU Client**
```bash
ssh pi@raspberrypi.local
cd ~/someip_rpc_project/window_rpc/build
VSOMEIP_CONFIGURATION=../helloworld-local.json VSOMEIP_APPLICATION_NAME=hello_world_client ./climate_ecu
```
```
Window position received: 30 %
```

**Terminal 5 — Speed Client**
```bash
ssh pi@raspberrypi.local
python3 ~/someip_rpc_project/window_rpc/speed_client.py
```
```
Vehicle speed received: 60 km/h
```

---

## 11. Debugging Journey & Issues Faced

Every issue we hit during the project is documented here in the order it was encountered.

---

### Issue 1 — Wrong Source File Names in CMakeLists.txt

After renaming all files from `hello_world_*` to `window_ecu` and `climate_ecu`, the CMakeLists.txt still referenced the old names:

```
Cannot find source file: hello_world_client_main.cpp
```

**Fix:** Updated CMakeLists.txt build targets to point to the new filenames.

---

### Issue 2 — Wrong Header File Names in Include Statements

The `.cpp` files still had:
```cpp
#include "hello_world_service.hpp"
```

**Fix:** Changed all includes to match the new header names:
```cpp
#include "window_ecu.hpp"
#include "climate_ecu.hpp"
```

---

### Issue 3 — Broken on_message_cbk Callback

Parts of the response creation block were accidentally deleted during refactoring:
```
expected primary-expression before '/'
resp was not declared in this scope
```

**Fix:** Manually reconstructed the full callback including `create_response()`, payload vector construction, and `app_->send(resp)`.

---

### Issue 4 — someipy Daemon Crash: No Such Device

When first running `someipyd`, it crashed with:
```
OSError: [Errno 19] No such device
```

**Root cause:** The `someipyd.json` had a hardcoded IP (`134.86.56.94`) that was no longer the Pi's actual address. The Pi's IP changes dynamically.

**Fix:** Ran `ip addr` to find the current IP on `eth0` (`100.88.162.130`) and updated all config files accordingly.

---

### Issue 5 — Wrong Command to Start someipy Daemon

Multiple attempts to start the daemon failed:
```bash
python3 -m someipy.daemon        # No module named someipy.daemon
someipy-daemon                   # command not found
python3 -m someipyd.json         # ModuleNotFoundError: No module named 'someipyd'
```

**Fix:** The correct command installed by someipy 2.1.2 is:
```bash
someipyd --config someipyd.json
```

---

### Issue 6 — Speed ECU Used Outdated someipy API

The original Speed ECU code was written for an older someipy API and failed with:
```
AttributeError: type object 'ServerServiceInstance' has no attribute 'create'
```

**Fix:** Reverted to the original `ServerServiceInstance()` constructor format which is correct in someipy 2.1.2. The issue was that we had accidentally overwritten the file with a wrong version. The key insight was that `connect_to_someipy_daemon()` is correct — it just requires the daemon to already be running.

---

### Issue 7 — Speed Client: service_found() Method Does Not Exist

The speed client script called:
```python
while not speed_client.service_found():
```

Which raised:
```
AttributeError: 'ClientServiceInstance' object has no attribute 'service_found'
```

**Fix:** Replaced the polling loop with a simple `await asyncio.sleep(3)` to give service discovery time to complete before making the method call.

---

### Issue 8 — GitHub: Password Authentication Not Supported

```
remote: Invalid username or token. Password authentication is not supported.
fatal: Authentication failed
```

**Root cause 1:** GitHub discontinued password-based git authentication — a Personal Access Token (PAT) is now required.

**Root cause 2:** A Fine-grained token was generated instead of a Classic token, causing further issues.

**Fix:** Generated a Classic PAT (GitHub → Settings → Developer Settings → Tokens (classic) → tick `repo`), then embedded it in the remote URL:
```bash
git remote set-url origin https://USERNAME:TOKEN@github.com/USERNAME/REPO.git
```

---

### Issue 9 — Git Push Rejected: Remote Ahead of Local

```
! [rejected] main -> main (fetch first)
Updates were rejected because the remote contains work that you do not have locally.
```

**Fix:**
```bash
git pull origin main --allow-unrelated-histories
git push origin main --force
```

---

### Issue 10 — README Merge Conflict

When merging the Speed ECU repository into the main repo, both had a `README.md`:
```
CONFLICT (add/add): Merge conflict in README.md
Automatic merge failed; fix conflicts and then commit the result.
```

**Fix:** Opened the file in nano, removed the conflict markers (`<<<<<<<`, `=======`, `>>>>>>>`), kept the desired content, and committed:
```bash
git add README.md
git commit -m "Merge Speed ECU - resolve README conflict"
git push origin main --force
```

---

## 12. Final Outputs

When the full system runs correctly, each terminal prints:

| Terminal | ECU | Final Output |
|---|---|---|
| 1 | someipy daemon | `Unix domain socket server started` |
| 2 | Speed ECU | `Offering GetVehicleSpeed() RPC service...` |
| 3 | Window ECU | Service offered, waiting for requests |
| 4 | Climate ECU | `Window position received: 30 %` |
| 5 | Speed Client | `Vehicle speed received: 60 km/h` |

---

## 13. Key Concepts Demonstrated

- **Cross-middleware SOME/IP integration** — vSomeIP (C++) and someipy (Python) running simultaneously on the same machine
- **RPC request/response** pattern — on-demand communication between ECUs
- **SOME/IP Service Discovery** — clients automatically find services at runtime
- **Payload serialization** — values transmitted as raw byte vectors
- **someipy daemon architecture** — Python SOME/IP apps offload network handling to a central daemon
- **Service-oriented automotive architecture** — each ECU offers a discoverable service
- **Multi-terminal distributed system** — 5 processes running concurrently on a single Pi
- **Automotive middleware concepts** relevant to AUTOSAR Adaptive and zonal ECU architectures

---

## 14. Future Improvements

- Dynamic window position and vehicle speed values from simulated sensors
- Speed-dependent window control: Speed ECU triggers Window ECU automatically
- Multiple concurrent clients requesting the same service
- Structured payloads using someipy's `SomeIpPayload` serialization
- Services that run continuously rather than single request/response cycles
- Systemd service files so all ECUs auto-start on Raspberry Pi boot
- A unified launcher script to start all 5 processes automatically
