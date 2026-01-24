# ESP32-C3 Relay Module - Test Application

This directory contains a test application that runs unit tests for all components on the target hardware using the Unity test framework.

## Running Tests

### Build and Flash Tests

```bash
cd test_app
idf.py build flash monitor
```

### Expected Output

You should see test results in the serial monitor:

```
====================================
ESP32-C3 Relay Module - Unit Tests
====================================

Running LED Control Tests...
test_led_init_returns_ok:PASS
test_led_init_idempotent:PASS
... (more tests)

-----------------------
18 Tests 0 Failures 0 Ignored
OK

====================================
All tests completed
====================================
```

## Adding New Tests

### 1. Convert Test File

Each component test file needs:
- A `run_<component>_tests()` function (not `app_main()`)
- A corresponding header file declaring the runner function

Example (`components/button_input/test/test_button_input.h`):
```c
#ifndef TEST_BUTTON_INPUT_H
#define TEST_BUTTON_INPUT_H

void run_button_input_tests(void);

#endif
```

In the test `.c` file, replace:
```c
void app_main(void) {
    UNITY_BEGIN();
    // tests...
    UNITY_END();
}
```

With:
```c
void run_button_input_tests(void) {
    // tests... (no UNITY_BEGIN/END needed)
}
```

### 2. Update test_app

1. Edit `test_app/main/CMakeLists.txt` - add source file and include directory
2. Edit `test_app/main/test_main.c` - include header and call runner function

### 3. Rebuild and Run

```bash
cd test_app
idf.py build flash monitor
```

## Structure

```
test_app/
├── CMakeLists.txt           # Project configuration
├── main/
│   ├── CMakeLists.txt       # Main component config (lists all test files)
│   └── test_main.c          # Test runner that calls all component tests
├── sdkconfig.defaults       # Default configuration
└── README.md                # This file
```

## Notes

- Tests run on actual hardware (ESP32-C3)
- All component implementations are tested in their real environment
- Tests use the Unity framework from ESP-IDF
- The main firmware is in `../` (parent directory)
