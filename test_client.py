import socket
import struct
import time

# Command definitions matching ipc_packet.h
CMD_WRITE     = 0
CMD_READ_REQ  = 1
CMD_READ_RESP = 2
CMD_IRQ       = 3

# Connect to the DPI-C bridge socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
print("Connecting to RTL Simulation...")
s.connect("/tmp/qemu_rtl.sock")
print("Connected!\n")

def send_write(addr, data):
    # Pack 3 uint32_t variables into exactly 12 bytes using Little-Endian ('<III')
    pkt = struct.pack("<III", CMD_WRITE, addr, data)
    s.sendall(pkt)

def send_read(addr):
    pkt = struct.pack("<III", CMD_READ_REQ, addr, 0)
    s.sendall(pkt)
    # Wait synchronously for the RTL to reply (Down path -> Up path loop)
    resp = s.recv(12)
    cmd, r_addr, data = struct.unpack("<III", resp)
    return hex(data)

# ---------------------------------------------------------
# Test 1: Read default value of Scratchpad (Address 0x00)
# ---------------------------------------------------------
print("Test 1: Read Scratchpad Default")
val = send_read(0x00)
print(f"  -> Got {val} (Expected: 0xa5a50000)\n")

# ---------------------------------------------------------
# Test 2: Write new value, then read it back
# ---------------------------------------------------------
print("Test 2: Write 0x12345678 to Scratchpad")
send_write(0x00, 0x12345678)
val = send_read(0x00)
print(f"  -> Read back: {val} (Expected: 0x12345678)\n")

# ---------------------------------------------------------
# Test 3: Trigger Asynchronous Interrupt (Address 0x04)
# ---------------------------------------------------------
print("Test 3: Trigger IRQ from RTL")
send_write(0x04, 0x01) # Bit 0 high triggers interrupt pulse

# Wait for the interrupt packet to arrive (Up path asynchronous)
irq_pkt = s.recv(12)
cmd, _, _ = struct.unpack("<III", irq_pkt)
if cmd == CMD_IRQ:
    print("  -> [SUCCESS] Hardware Interrupt (IRQ) received from RTL!\n")

s.close()
