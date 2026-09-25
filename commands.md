```markdown
# QEMU & Verilator Co-Simulation Command Cheat Sheet

## 1. Directory Setup & Source Cloning
```bash
# Create and enter the project directory
mkdir -p ~/QEMU_Customize_HW
cd ~/QEMU_Customize_HW

# Clone QEMU source code
git clone [https://gitlab.com/qemu-project/qemu.git](https://gitlab.com/qemu-project/qemu.git)

```

## 2. RTL & DPI-C Bridge (Verilator)

```bash
# Navigate to the project root
cd ~/QEMU_Customize_HW

# (Optional) Syntax check for C header and SystemVerilog DUT
gcc -Wall -Wextra -fsyntax-only ipc_packet.h
verilator --lint-only -Wall dut.sv

# Clean previous builds
rm -rf obj_dir

# Compile SystemVerilog and C into a C++ simulation executable
verilator -Wall --cc --exe --build -j 0 --top-module tb_top dut.sv tb_top.sv dpi_bridge.c sim_main.cpp -CFLAGS "-I."

# Run the Verilator simulation (leave this running in Terminal 1)
./obj_dir/Vtb_top

```

## 3. Python Verification Client (Optional)

```bash
# Run the Python test script (in a separate terminal)
cd ~/QEMU_Customize_HW
python3 test_client.py

```

## 4. QEMU Custom Device & Compilation

```bash
# Navigate to QEMU custom hardware directory to create the device file
cd ~/QEMU_Customize_HW/qemu/hw/misc/
gedit rtl_bridge_pci.c

# Edit the build system file to include the new device
gedit meson.build
# Add: system_ss.add(when: 'CONFIG_PCI', if_true: files('rtl_bridge_pci.c'))

# Configure QEMU build for x86_64 to save compilation time
cd ~/QEMU_Customize_HW/qemu
./configure --target-list=x86_64-softmmu

# Compile QEMU using all available CPU cores
make -j$(nproc)

```

## 5. End-to-End Execution & Testing

### Terminal 1: Run Verilator

```bash
cd ~/QEMU_Customize_HW
./obj_dir/Vtb_top

```

### Terminal 2: Run QEMU in Monitor Mode

```bash
cd ~/QEMU_Customize_HW/qemu/build
./qemu-system-x86_64 -m 512M -machine q35 -device rtl-bridge-pci -display none -monitor stdio

```

### QEMU Monitor Commands (Inside Terminal 2)

```text
(qemu) info pci                     # Find the BAR0 address of device 1234:5678
(qemu) xp /1xw 0xfebd5000           # Read 32-bit hex word from BAR0 (replace address as needed)
(qemu) gdbserver                    # Open GDB port on tcp::1234
(qemu) quit                         # Exit QEMU

```

### Terminal 3: GDB (Hardware Writes & Interrupts)

```bash
gdb

```

```text
(gdb) target remote localhost:1234                    # Connect to QEMU
(gdb) set *(unsigned int *)0xfebd5000 = 0x12345678    # Write to Scratchpad (Offset 0x00)
(gdb) set *(unsigned int *)0xfebd5004 = 0x1           # Write to Control Reg (Offset 0x04) to trigger IRQ

```

## 6. Document Generation (Pandoc)

```bash
# Install Pandoc and PDF engine
sudo apt update
sudo apt install pandoc wkhtmltopdf

# Convert Markdown guide to DOCX
pandoc bridge_guide.md -o "QEMU to Verilator RTL Co-Simulation Bridge.docx"

# Convert Markdown guide to PDF
pandoc bridge_guide.md -o "QEMU to Verilator RTL Co-Simulation Bridge.pdf"

```
