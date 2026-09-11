# RepSpectre

RepSpectre is a smart workout tracker designed to attach to a barbell instead of a wrist, providing more direct information about the barbell's movement for measuring and analyzing repetition data.

The long-term goal is for RepSpectre to act as a quiet workout companion that sits on the barbell and tracks a training session without requiring constant user interaction. It should be able to follow the session in the background, understanding what the barbell is being used for and recording relevant workout information. The user can then interact with it through the mobile application or an additional screen when needed.

> **Note**: These parts are still a work in progress; current development is focused on model development to properly distinguish between different movement states.

The project is intended to understand enough of a training session to distinguish ordinary bar handling, lift setup, and repetitions worth recording.

This repository contains the portable firmware for that work: it collects and buffers IMU data, manages Bluetooth Low Energy state, and coordinates local state machines and inference on sensor data to predict the device's current motion state.

ESP32 (using ESP-IDF) is the first implementation target, while the application core and platform code remain separate so the same system can be ported to another microcontroller.

## Project context

The project is split across three repositories that carry barbell-motion data from collection to embedded experiments:

For now, while model creation is the focus, `repspectre-lite` is used as a simple data-capture device that sends sensor data over BLE. The portable `repspectre` architecture is developed alongside that collection workflow.

```text
-> repspectre: Captures and processes barbell motion on-device
        |
        v
   repspectre-mobile: Stores labelled workout sessions
        |
        v
   repspectre-ml: Prepares, inspects, and trains on those sessions
```

[repspectre-lite](https://github.com/MuzammilAijaz/repspectre-lite) remains the faster ESP-IDF prototype used for early hardware and data-collection work. The lighter project includes facilities for on-device inference. The model remains in development, so these features are mainly used for testing and are disabled during data collection.

[repspectre-mobile](https://github.com/MuzammilAijaz/repspectre-mobile) receives and labels recordings on Android.

[repspectre-ml](https://github.com/MuzammilAijaz/repspectre-ml) prepares those recordings for analysis and model training.

### Current direction

> **Project status:** the Active Object architecture, sensor path, windowing, Bluetooth lifecycle, and test infrastructure are under active development. The current `MotionInferenceAO` is rule-based motion-state logic. A trained TensorFlow Lite Micro model is not yet integrated into this firmware path.

Upcoming work includes model-to-firmware integration, backpressure behavior when processing falls behind acquisition, and further platform ports.

Motion state will later determine when a BLE connection receives detailed telemetry; that policy remains incomplete. The target-specific layer is deliberately isolated so a port can replace board support, drivers, and startup code without rewriting the application state machines.

## Internal State Maintenance

A sensor stream alone does not explain what a person is doing. Similar movement can occur while fitting the device, carrying the bar, preparing to lift, or completing a repetition. The meaning of a new reading depends on the events that came before it.

The firmware is therefore intended to maintain a local history of the workout through its states. That history can later guide decisions such as when to collect a window for closer analysis, when detailed telemetry is useful, and when Bluetooth can remain inactive. Keeping this context on the device also avoids treating the barbell unit as a transparent sensor relay whose interpretation exists only on a connected phone.

## Design choices

### Event-driven state machines using QP/C

QP/C was selected to provide an established event-driven and state-machine framework. Its Active Object model gives each module an explicit event queue and a clear ownership boundary.

An application built directly from blocking tasks, semaphores, and mutexes can still be made to work. As the number of modules grows, however, the coordination logic becomes harder to follow, test, and change. The firmware uses event-driven modules so their interactions can be described as state transitions and events instead of being scattered across task synchronization code.

### QP/C and FreeRTOS as the kernel

On MCU targets, the QP/C port runs Active Objects on FreeRTOS. Starting an Active Object creates its static FreeRTOS event queue and pinned FreeRTOS task, while ESP32 platform components use FreeRTOS directly where target integration requires it.

This does not apply to every build path. The default host build uses QP/C's POSIX pthread port, behavior tests use the POSIX QUTest port, and Arduino HIL tests use a dedicated Arduino QUTest port. The portable core expresses its module coordination through QP/C events so the application logic does not depend on one target kernel.

### State-guided inference

The current system already separates sensor acquisition, windowing, sequencing of the active objects, Bluetooth lifecycle, and rule-based motion inference.

The longer-term direction is to create a state machine primarily for maintaining the current position or state of the device as a policy input: low-activity states can reduce work by using a simpler ML model or algorithm, while lift-related states can request richer data or inference.

## Architecture

```text
components/
  core/                 Portable Active Objects, module events, and HAL interfaces
  drivers/              Shared driver code
  platform/             ESP32, Arduino, and common platform implementations
main/                   Application entry point
tests/
  unit/                 CppUTest module tests
  behavior/             QUTest/QSPY behavior tests
  hil/arduino/          Arduino-target HIL runners and sketches
tools/esp32/apps/       Focused ESP32 diagnostic applications
```

The portable application layer lives under `components/core`. Platform implementations live separately under `components/platform`.


| Established module | Responsibility |
| --- | --- |
| `SequencerAO` | Drives boot order, starts dependent modules, tracks readiness, and controls the main Bluetooth operating states |
| `SensorAO` | Initializes the sensor and transfers FIFO samples into memory leased by the windowing module |
| `WindowAO` | Maintains overlapping logical windows over a shared IMU sample ring buffer |
| `BluetoothAO` | Manages Bluetooth initialization, advertising, connection state, and bridge events |

| Module in development | Responsibility |
| --- | --- |
| `MotionInferenceAO` | Runs the current rule-based lift-phase state machine and publishes motion-state changes and rep counts |

| Planned Modules | Responsibility |
| --- | --- |
| `MotionStateAO` | Will maintain a system-level view of device and motion state, including power policy and inference selection |
| `LightInferenceAO` | Will run lower-cost heuristics or classical models during low-power operating states |
| `InferenceAO` | Will host neural-model inference for motion patterns that need more context |


`WindowAO` owns a fixed ring buffer and exposes logical windows into that buffer. `SensorAO` receives a write location, fills it from the IMU FIFO, and signals completion. This avoids allocating or copying a new buffer for every inference-sized window.

Hardware boundaries are represented by interface structs such as `SensorInterface` and `BspInterface`. A target port supplies these implementations, while the core modules remain unaware of ESP-IDF, Arduino, or another target SDK.

The focused applications under `tools/` assemble existing modules into small diagnostic programs. They are used to investigate a specific target concern without turning the main application into a collection of temporary experiments.

## Build and test

The project uses CMake for the host-side build. The Makefile provides shortcuts for configuring, building, cleaning, and running the different types of tests.

QP/C is fetched through CMake during the first configuration. The first build therefore requires network access unless the dependency has already been populated in the CMake dependency cache.

### Host build

The normal build compiles the host-side C/C++ project, including the portable application components and the targets defined under components/, main/, and tests/. It does not build the ESP32 firmware image.

```
# Configure and build the host project
make build
```

The build is configured as a Debug build with compile commands enabled. The generated build files and artifacts are placed in build/.

To remove compiled artifacts while keeping the CMake build directory:

```
make clean
```

To remove the entire build directory and configure everything again from scratch:

```
make fullclean
make build
```

### Unit tests

The default test command runs the unit-test suite on the host:

```
make test
```

The repository also provides a command for running all CTest targets:

```
make test-all
```

This first builds the project and then runs CTest with output shown for failing tests.

### QUTest / behavior tests

The project uses QUTest and QSPY for behavior-level tests of the state-machine and event-driven parts of the system. These tests can exercise the application logic through the QP/QUTest port without requiring the actual MCU.

The QUTest tooling is configured through:

```
QSPY_BIN
QUTEST_PY
```

The paths default to locations under `$HOME/qtools`, but can be overridden when invoking `make`.

### Hardware-in-the-loop tests

HIL tests are used when behavior needs to be tested on an actual Arduino-compatible target rather than only on the host.

The HIL test suite can be run with:

```
make test-hil
```

Note: HIL tests are explicitly registered with CTest and labelled hil; make test-hil runs the HIL tests that are enabled for the current build configuration. It does not automatically run all tests in the repository on hardware. Currently, there is no separate feature for selectively enabling or disabling individual HIL tests, so the set of HIL tests is determined by their registration in the build configuration.

HIL tests require a compatible target board, the Arduino build environment, QSPY, QUTest, and an available serial port. The target board and tool locations can be overridden when invoking make:

```
make test-hil \
  ARDUINO_FQBN=esp32:esp32:esp32s3 \
  ARDUINO_PORT=/dev/ttyACM0 \
  QSPY_BIN=/path/to/qspy \
  QUTEST_PY=/path/to/qutest.py
  ```


The HIL setup uses the Arduino build environment to compile and upload the test sketch, while QUTest/QSPY is used to drive and observe the behavior of the application on the board.

Individual stages of the HIL workflow can also be run separately:

```
# Configure HIL support
make configure-hil

# Compile the test sketch
make test-hil-build

# Upload the compiled sketch
make test-hil-flash

# Run the QUTest script against the board
make test-hil-run
```

This separation is useful when developing a HIL test because the sketch does not need to be rebuilt or uploaded every time the QUTest script is changed.
See tests/hil/arduino/CMakeLists.txt for more context.

### Building tools

The repository contains focused tools that assemble selected parts of the portable firmware with a platform implementation. These tools are useful when a specific part of the system needs to be observed on hardware, such as checking sensor data collection or Bluetooth behavior, without requiring the complete workout-tracking system to be finished first. They also provide a place to build and deploy a particular hardware experiment while the main application architecture is still being developed.

The current hardware tools use ESP-IDF. They have a separate build and deployment flow because ESP-IDF projects are configured through their own component system and `idf.py`, while the normal `make build` target configures the host-side CMake project for portable code and tests.

For example, the repository currently provides a deployment target for the `SensorDataBeacon` tool:

```
make tool-SDB-esp-deploy
```

This invokes ESP-IDF's `idf.py` to build, flash, and monitor the tool. The same approach can later be used for another target platform by providing the corresponding platform implementation and build integration.

## Testing approach

Testing is layered because failures differ at each level:

- Unit tests use CppUTest with fakes and stubs for individual modules.
- Behavior tests use QUTest and QSPY to inspect event-driven behavior without a physical board.
- HIL tests exercise sensor, Bluetooth, sequencer, and windowing paths against an attached device. Some target driver and board behavior directly; others exercise several modules together.

The test tree mirrors the application structure so a module and its supporting fakes remain close together. Hardware experiments are kept under `tests/arduino-experiments`; they are useful for validating assumptions quickly, but are not treated as the portable application architecture.
