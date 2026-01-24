# ESP32-C3 Relay Module - Test Application
# This project runs unit tests on the target hardware

cmake_minimum_required(VERSION 3.16)

# Set the components directory to include the parent project's components
set(EXTRA_COMPONENT_DIRS "../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32c3-relay-tests)
