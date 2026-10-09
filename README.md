# QNX_Remote_Patient_Monitoring
Fault-Tolerant Remote Patient Monitoring System with Priority Alarm Recovery using QNX RTOS

## 1. Project Overview

QNXMedLink is a remote patient monitoring prototype developed using C programming and QNX Neutrino RTOS concepts. It simulates patient health readings, assigns alarm priorities, buffers patient records, and transmits them to a central monitoring station over TCP.

The project focuses on improving monitoring reliability during network interruptions and demonstrating priority-based alarm handling and data recovery concepts.

## 2. Problem Statement

Remote patient monitoring systems can experience network disconnections, delayed alarms, buffer overflow, or device restarts. These failures can interrupt the transmission of patient records and affect monitoring reliability.

This project explores local buffering, priority-based transmission, alarm handling, acknowledgements, and reconnection to reduce the impact of communication failures.

## 3. Objectives

* Simulate health readings for multiple patients.
* Classify readings into NORMAL, WARNING, and CRITICAL priorities.
* Generate a local alarm for critical readings when the network is unavailable.
* Store pending records in a buffer during temporary communication failures.
* Transmit records to a central station using TCP.
* Remove buffered records after receiving an acknowledgement.
* Explore recovery and reliability improvements for a real-time monitoring system.

## 4. System Architecture

Patient Node
↓
QNX Timer Pulse and Patient Data Generation
↓
Health Parameter and Priority Classification
↓
Local Priority Buffer
↓
Network Communication Thread
↓ TCP
Central Monitoring Station
↓
Patient Record Display and Central Alarm
↓
ACK Sent to Patient Node

## 5. Technologies Used

* Operating System: QNX Neutrino RTOS (target environment)
* Programming Language: C
* Development Environment: QNX Momentics IDE
* Networking: TCP sockets
* Concurrency: POSIX threads, mutexes, and condition variables
* Timing and Messaging: QNX channels, pulses, and timer APIs

## 6. Main Components

### Patient Node

* Simulates four patients.
* Generates heart rate, SpO2, and temperature readings.
* Assigns priority based on configured thresholds.
* Stores records in a RAM-based priority buffer.
* Attempts to connect to the central station.
* Sends records and waits for ACKs.
* Activates a local critical alarm when the network is unavailable.

### Central Station

* Listens for connections on TCP port 5000.
* Receives patient records.
* Displays patient readings, timestamps, and priorities.
* Raises an alarm when a critical record is received.
* Sends ACK messages to the patient node.

## 7. Fault Scenarios Under Study

* Central station unavailable
* Network disconnection during transmission
* Pending records awaiting transmission
* Buffer reaching its capacity
* ACK loss and possible duplicate transmission
* Application restart and persistence of pending records

## 8. Current Implementation Status

The patient-node and central-station programs implement simulated patient readings, priority classification, in-memory buffering, TCP communication, acknowledgement handling, and alarm display.

Persistent recovery across device or process restarts, robust handling of partial TCP transfers, and safe buffer-overflow handling require further implementation and testing.

## 9. Testing Plan

* Verify normal, warning, and critical readings.
* Verify local alarms during network failure.
* Verify record transmission and ACK handling.
* Disconnect and reconnect the central station.
* Test behavior when the buffer reaches capacity.
* Test restart recovery after persistent storage is implemented.
* Record observed results and screenshots.

## 10. Repository Structure

* `src/central_station.c` — central monitoring server
* `src/patient_node.c` — patient simulation and transmission
* `docs/` — project documentation and architecture
* `tests/` — test plan and results
* `screenshots/` — development and test evidence

## 11. Limitations

* Patient readings are simulated and are not actual medical measurements.
* The current buffer uses RAM and does not guarantee recovery after a restart.
* The implementation needs robust TCP partial-transfer handling and improved duplicate-record protection.
* The configured thresholds are for prototype demonstration and are not clinical diagnostic rules.

## 12. Development Environment
* QNX Momentics IDE
* VM Ware Virtual Machine

## 12. Future Enhancements

* Persistent storage for pending patient records
* Reliable message framing and complete TCP send/receive loops
* Duplicate detection using patient ID and sequence number
* Improved buffer-overflow handling that protects critical records
* Testing on the intended QNX target
* Integration with suitable sensors and a monitoring dashboard

## 13. Team Members
BATHINA BHAVYA SRI LAKSHMI - Coding and System Development
ALLANKA SRI HARSHITHA - Central Station and Testing
MANGI RISHITHA SRAVANTHI - Documentation, GitHub Repository and Presentation

## 14. Disclaimer

This is an educational prototype for a hackathon. It is not a certified medical device and must not be used for diagnosis or treatment.
