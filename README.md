# DreamStella

[![KallistiOS](https://img.shields.io/badge/Powered%20by-KallistiOS-blue.svg)](https://github.com/KallistiOS/KallistiOS)
[![Platform](https://img.shields.io/badge/Platform-Sega%20Dreamcast-lightgrey.svg)](https://en.wikipedia.org/wiki/Dreamcast)
[![License](https://img.shields.io/badge/License-GPLv2-green.svg)](LICENSE)

An optimized, low-latency Atari 2600 emulator based on the Stella 3.9.3 core, designed to run on Sega Dreamcast hardware.

The goal of this project is to run classic Atari 2600 games with high accuracy while extracting the maximum possible performance from the Sega Dreamcast console. Currently, DreamStella ensures stable frame rates (60 FPS for NTSC / 50 FPS for PAL) across the vast majority of the game library.

---

## Key Features

* **Bare-Metal Rendering:** Replaces high-level abstraction libraries (such as SDL) with direct pixel injection into VRAM, utilizing the Dreamcast's native **Store Queues (SQ)**.
* **Low Input Latency:** Optimized control polling to ensure immediate response times without input lag.
* **Lightweight UI:** Uses native bitmap font rendering via **BMFont** and ultra-fast texture loading with **stb_image**, keeping the executable size and RAM usage to an absolute minimum.
* **Region Detection:** Automatic display adjustments and speed timing for NTSC, PAL, and SECAM ROMs.

---

## Technology Stack

This project is built using modern tools tailored for the Dreamcast homebrew scene:

* **[KallistiOS (KOS)](https://github.com/KallistiOS/KallistiOS):** The definitive open-source operating system for the Sega Dreamcast.
* **[stb_image](https://github.com/nothings/stb):** A public domain library for efficient loading of UI assets.
* **[BMFont](https://www.angelcode.com/products/bmfont/):** Bitmap font rendering to avoid the real-time CPU overhead associated with FreeType/TTF libraries.
* **[Stella Emulator](https://stella-emu.github.io/).** Stella: "A Multi-Platform Atari 2600 VCS Emulator"

---

## Under the Hood

Adapting the Stella core for a 1998 RISC CPU required specific architectural care. Rather than a simple port, DreamStella focuses on **Performance-First Design**. 

Heavy abstractions from standard C++ (such as `<iostream>` and stream-based I/O classes) have been removed from critical emulation paths. Memory read loops in the TIA module have also been refactored to ensure better performance on the Sega Dreamcast console.

---

## Current Status & Roadmap

This project is an ongoing journey to emulate the entire Atari 2600 library flawlessly on the Dreamcast. Although the emulator is highly optimized and offers a perfectly playable experience for almost all titles, achieving a stable, locked 60 FPS on the most brutally demanding cartridges—such as those using the DPC coprocessor (e.g., *Pitfall II*)—remains a constant challenge.

This is an ever-evolving project, and there is always room for hardware-level optimizations.

---

## How to Build

### Prerequisites
You will need a configured Dreamcast development environment. I recommend using **[DreamSDK](https://dreamsdk.org/)** (for Windows) or manually building the KallistiOS toolchain (for Linux/macOS).

### Instructions
1. Clone the repository to your local machine:
   ```bash
   git clone https://github.com/aleanjos/dreamstella.git

2. Navigate to the `dc` folder and run `make` to generate the `.cdi` file, `make debug` to generate the `.elf` file, or `make clean` to clean up all files generated during compilation:
   ```bash
   cd dc
   make
   
