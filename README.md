This repository contains the firmware for the Right Headlight Control Module (HCM), operating as a Slave Node within this decentralized Adaptive Front-lighting System (AFS).

Powered by an ESP32, this module decodes real-time broadcast and specific control frames sent from the Main ECU (Master Node) via UART to independently orchestrate its local lighting components and mechanical actuators.

Key Features
Decentralized Execution: Operates as an autonomous hardware driver node, keeping the Main ECU decoupled from low-level PWM timing, servo actuation, and pixel animations.

Mirrored 2-Axis Mechanical Actuation: Independently commands two high-torque 180° servo motors for active dual-axis beam orientation:

Ox Axis (Swivel/Cornering): Maps horizontally to match the steering wheel's angular displacement.

Oy Axis (Inverted Dynamic Leveling): Adjusts vertically using an Exponential Moving Average (EMA) filter. The software mapping is structurally mirrored compared to the Left HCM to ensure symmetrical leveling when adjusting for vehicle pitch.

Smart Power Lighting Control: Drives high-power LED arrays (Low Beam, High Beam, and Fog Light) using high-frequency (5000Hz) 8-bit PWM signals via dedicated constant-current LED drivers (PT4115).

Non-Blocking Dynamic Animations: Uses an asynchronous millis() framework for the progressive sequential turn signal animation (orange swept effect) and a smooth cornering fade-in/fade-out routine on the fog light.

Hardware Stack
Microcontroller: ESP32 (NodeMCU)

Actuators: 2x 180° High-Precision Servo Motors (Ox and Oy orientation)

LED Driver Stage: PT4115 Constant-Current Buck Controllers (PWM driven)

Optics & Lighting: WS2812B Addressable LED strip (for DRL/Turn signals), High-Power LED Projector Lenses (Low/High/Fog)

Comms: UART (Serial2 Interface) mapped to GPIO pins 16 (RX) and 17 (TX)

Note: This firmware is hardcoded with a unique node identifier (ID_PROPRIU 0x01) to selectively filter and execute packets meant specifically for the right side of the vehicle.
