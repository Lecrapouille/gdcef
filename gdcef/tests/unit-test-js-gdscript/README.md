# JavaScript-GDScript Bindings Test

This directory contains a test suite for the bidirectional communication between JavaScript and GDScript using GDCef.

## Purpose

The purpose of this test is to verify that data can be correctly passed between the JavaScript environment in CEF and the GDScript environment in Godot. It tests various data types and structures to ensure they are properly serialized, transmitted, and deserialized.

## Components

The test consists of two main components:

1. **GDScript Side** (`test_bindings.gd`):
   - Initializes CEF
   - Creates a browser that loads the HTML test page
   - Registers a GDScript method that can be called from JavaScript
   - Handles data received from JavaScript and sends it back

2. **JavaScript Side** (`test_bindings.html`):
   - Provides a UI for testing different data types
   - Sends data to Godot using `window.godotMethods.test_bindings()`
   - Receives data back from Godot via `window.godotEvents.on('test_result')`
   - Compares the sent and received data for equality

## Data Types Tested

The test verifies the correct transmission of these data types:

- Null values
- Booleans
- Integers
- Floating point numbers
- Strings
- Arrays
- Objects (dictionaries)
- Binary data (as PackedByteArray/Uint8Array)
- Nested structures (objects inside objects, arrays inside arrays)
- Special cases (very long strings, special characters)

## How to Run the Test

1. Open the scene for this test in Godot
2. Run the scene
3. The test interface will appear
4. Click on different buttons to test different data types
5. Check the results displayed on screen

## Expected Results

The test verifies that:

- Each value sent from JavaScript to GDScript is correctly received
- The same value is sent back from GDScript to JavaScript
- The received value matches the original value (deep comparison)

For each test, a status indicator shows whether the test passed or failed.

## Using This as a Reference

This test suite serves as a comprehensive reference for how to implement bidirectional communication between JavaScript and GDScript. You can refer to the code here when implementing your own communication patterns in your GDCef-powered Godot applications.
