#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "qemu/error-report.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/qemu_rtl.sock"

typedef enum {
    CMD_WRITE     = 0,
    CMD_READ_REQ  = 1,
    CMD_READ_RESP = 2,
    CMD_IRQ       = 3
} cmd_type_t;

typedef struct {
    uint32_t cmd;
    uint32_t addr;
    uint32_t data;
} __attribute__((packed)) rtl_packet_t;

#define TYPE_RTL_BRIDGE_PCI "rtl-bridge-pci"
OBJECT_DECLARE_SIMPLE_TYPE(RTLBridgePCIState, RTL_BRIDGE_PCI)

struct RTLBridgePCIState {
    PCIDevice parent_obj;
    MemoryRegion mmio;
    int sock_fd;
};

static void rtl_pci_connect(RTLBridgePCIState *s) {
    if (s->sock_fd >= 0) return;

    s->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(s->sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s->sock_fd);
        s->sock_fd = -1;
    }
}

static uint64_t rtl_pci_read(void *opaque, hwaddr offset, unsigned size) {
    RTLBridgePCIState *s = opaque;
    rtl_pci_connect(s);
    if (s->sock_fd < 0) return 0xFFFFFFFF;

    rtl_packet_t req = { .cmd = CMD_READ_REQ, .addr = (uint32_t)offset, .data = 0 };
    if (write(s->sock_fd, &req, sizeof(req)) != sizeof(req)) return 0xFFFFFFFF;

    rtl_packet_t resp;
    while (1) {
        ssize_t n = read(s->sock_fd, &resp, sizeof(resp));
        if (n == sizeof(resp)) {
            if (resp.cmd == CMD_READ_RESP) {
                return (uint64_t)resp.data;
            } else if (resp.cmd == CMD_IRQ) {
            // Add this print statement!
                printf("\n\n[RTL-BRIDGE] >>> HARDWARE INTERRUPT (IRQ) RECEIVED FROM VERILATOR! <<<\n\n");
                pci_set_irq(&s->parent_obj, 1);
                pci_set_irq(&s->parent_obj, 0);
            }
        } else {
            break; 
        }
    }
    return 0xFFFFFFFF;
}

static void rtl_pci_write(void *opaque, hwaddr offset, uint64_t val, unsigned size) {
    RTLBridgePCIState *s = opaque;
    rtl_pci_connect(s);
    if (s->sock_fd < 0) return;

    rtl_packet_t req = {
        .cmd  = CMD_WRITE,
        .addr = (uint32_t)offset,
        .data = (uint32_t)val
    };
    
    // FIX: Check return value to prevent -Werror=unused-result
    if (write(s->sock_fd, &req, sizeof(req)) != sizeof(req)) {
        // Silently drop failed writes in this simple behavioral model
    }
}

static const MemoryRegionOps rtl_pci_ops = {
    .read = rtl_pci_read,
    .write = rtl_pci_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = { .min_access_size = 4, .max_access_size = 4 },
};

static void rtl_bridge_pci_realize(PCIDevice *pci_dev, Error **errp) {
    RTLBridgePCIState *s = RTL_BRIDGE_PCI(pci_dev);
    s->sock_fd = -1;
    
    memory_region_init_io(&s->mmio, OBJECT(s), &rtl_pci_ops, s, "rtl-bridge-mmio", 0x1000);
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);
    pci_config_set_interrupt_pin(pci_dev->config, 1); // 1 = INTA
}

// FIX: Changed void *data to const void *data
static void rtl_bridge_pci_class_init(ObjectClass *klass, const void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = rtl_bridge_pci_realize;
    k->vendor_id = 0x1234;     
    k->device_id = 0x5678;     
    k->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo rtl_bridge_pci_info = {
    .name          = TYPE_RTL_BRIDGE_PCI,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(RTLBridgePCIState),
    .class_init    = rtl_bridge_pci_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void rtl_bridge_pci_register_types(void) {
    type_register_static(&rtl_bridge_pci_info);
}
type_init(rtl_bridge_pci_register_types)
