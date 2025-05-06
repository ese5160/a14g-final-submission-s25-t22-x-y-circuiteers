[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/AlBFWSQg)
# a14g-final-submission

    * Team Number: T22
    * Team Name: X&Y Circuiteers
    * Team Members: Xiran Hu, Yuner Zhang
    * Github Repository URL: https://github.com/ese5160/a14g-final-submission-s25-t22-x-y-circuiteers
    * Description of test hardware: (development boards, sensors, actuators, laptop + OS, etc) : The laptop, SAM W25 Xplained Pro development board, SD card.

## 1. Video Presentation
Google Drive Link: [Final_Video_Presentation](https://drive.google.com/file/d/1SnDz7Wh9IL8G6lDfHzbvuNYeVGlQYDE1/view?usp=sharing).

## 2. Project Summary

### Device Description

We designed a four-legged robot named Bobi, capable of walking, striking playful poses, and monitoring its surroundings to provide timely alerts. Inspired by the longing for a pet, Bobi serves as both a lovable companion and a smart home assistant.

Our motivation stemmed from a desire for companionship and emotional comfort in everyday life. Bobi addresses this by acting as a friendly presence while helping users stay aware of their home environment.

Bobi enables real-time monitoring of **temperature**, **humidity**, **air quality**, and **obstacle distance** through an internet-connected system. Users can control Bobi via a **web-based interface**, and we support **OTAFU** (Over-The-Air Firmware Updates) for remote maintenance.

---

### Device Functionality

Bobi features two core functionalities: **movement** and **environment monitoring**.

- **Movement**:  
  A servo driver board connected to the SAMW25 microcontroller controls eight servo motors to operate Bobi’s legs. Pressing the **touch sensor** triggers a playful wiggle, while the **gesture sensor** detects hand waves to turn Bobi left or right.

- **Environment Monitoring**:  
  Bobi monitors **temperature**, **humidity**, **VOC levels (air quality)**, and **obstacle distance**. If any value exceeds the normal range, Bobi displays a warning on-screen and sounds a buzzer. For instance, if something is too close, Bobi beeps and moves backward.

- **NFC Integration**:  
  Tapping a phone on Bobi’s leg triggers a custom message display on the screen.

![Detailed System Block Diagram](Detailed%20System%20Block%20Diagram.png)

---

### Challenges

1. **Servo Coordination**:  
   Controlling eight servo motors simultaneously to enable smooth, lifelike movements was our first major challenge. We tested numerous angle-delay combinations to achieve natural motion.

2. **Memory Constraints**:  
   Integration revealed memory limitations when running all functions concurrently. To address this, we implemented **FreeRTOS** for efficient task scheduling and added a toggle switch to enable gesture detection only when needed, saving system memory.

---

### Prototype Learnings

Building Bobi taught us the importance of **co-planning hardware and software** from the start. Synchronizing movement control with sensor feedback was key to achieving responsive, realistic behavior.

We also discovered the value of **incremental testing**—debugging in small steps allowed us to identify and resolve issues early, ultimately improving the design.

If we were to redesign Bobi, we would:
- Choose a microcontroller with greater memory capacity.
- Strengthen the robot's frame for better stability.
- Develop a mobile app for enhanced control and user experience.

---

### Next Steps & Takeaways

For future iterations, we plan to:
- Add more gesture types for expanded interaction.
- Offload data processing to the cloud to reduce local memory usage.

Through **ESE5160**, we gained practical skills in:
- **PCB design**
- **Hardware debugging**
- **FreeRTOS task management**
- **Firmware updating**

This course gave us end-to-end experience in building a complete IoT system, blending embedded systems with real-world applications.

### Project Links
Frontend_Node_RED Link: http://52.224.120.179:1880/ui/#!/0?socketid=mmrCvhwkhdHmjPopAADz
Backend_Node_RED Link: http://52.224.120.179:1880/#flow/80e1a0e60252a46e

## 3. Hardware & Software Requirements

## 4. Project Photos & Screenshots

## Codebase

- A link to your final embedded C firmware codebases
- A link to your Node-RED dashboard code
- Links to any other software required for the functionality of your device

