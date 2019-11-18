![image](https://user-images.githubusercontent.com/407441/69074928-3637aa80-09e5-11ea-91d9-43b4f0ae26b8.png)

# Qnes
Qnes is a Famicom/NES emulator written in C++. It is based largely on experience writing a previous NES emulator and the incredible *NESDev wiki*. It relies on GLAD for GL extension handling, and on imgui for debugger and other interface goodness.

# System Fidelity and Mapper Support
I feel confident the CPU implementation of Qnes is virtually complete. The PPU is probably 85% there, though some scrolling and sprite-zero hit functionality is not completely accurate. The PPU is clocked in between instructions, but is not interleaved with the various CPU states for individual 6502 instructions (The CPU/PPU interaction is not cycle accurate -- but, I'm also unaware of anything that does/can depend on that level of fidelity.) There are a number of quirks that are not implemented, but are listed in the nesdev wiki.

Mapper 0 is complete, and a few other basic bank switching mappers like NROM and SxROM are close to complete, allowing some games to run completely, while not booting others. I do not anticipate writing many mappers, and would consider MMC3 a final goal in this respect.

# Building
Qnes requires `SDL2` for graphics (and eventually audio), and `scons` for building. The following should work on a modern ubuntu system
```
sudo apt install -y libsdl2-dev scons build-essential 
scons -j8
./build/qnes [path-to-your-nes-file]
```

# Basic Architecture
Qnes has only a few pieces, which are loosely modeled around the main components of the original system. There is a CPU, PPU, 'Bus' object that handles CPU bus access to other devices, Cartridge which is the high-level interface to various cartridge types, and Mappers which are forms of the various circuits that make up NES cartridges. 

The UI itself is very vanilla SDL2, and a debugger/memory editor written on top of ImGui (https://github.com/ocornut/imgui). 

There is a desire to refactor a little bit of this to better match the original hardware more closely.

# CPU and PPU Debuggers
The CPU debugger allows:
- Basic Step / Continue
- Execute breakpoints
- Jump PC to arbitrary address
- Disassembly which decoded addresses for every addressing mode
- Memory editor which allows you to view in real-time and edit the 2KB of main CPU RAM.

![image](https://user-images.githubusercontent.com/407441/69075030-7565fb80-09e5-11ea-8728-6d3f57ffda4a.png)

The PPU debugger allows:
- Viewing PPU RAM
- Viewing Pattern Tables (VRAM 0x0000 - 0x1FFF)
- Viewing the current Nametables, with an overlay for the current scroll

![image](https://user-images.githubusercontent.com/407441/69075070-8adb2580-09e5-11ea-84e3-e5ab8f9ff6df.png)

