// blk.h - simple virtio-blk interface

#ifndef _BLK_H_
#define _BLK_H_

#include <stddef.h>
#include <stdint.h>

#define V2P(a) ((uintptr_t)(a))

// --- 1. MMIO register definitions ---
#define VIRTIO_MMIO_START 0x10001000
// QEMU virt: up to 8 virtio-mmio devices at 0x10001000..0x10008000
#define VIRTIO_MMIO_END 0x10009000
#define VIRTIO_MMIO_STRIDE 0x1000

#define VIRTIO_MMIO_MAGIC_VALUE 0x000
// virtio-mmio version
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028 // V1 only
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c // V1 only
#define VIRTIO_MMIO_QUEUE_PFN 0x040   // V1 only
#define VIRTIO_MMIO_QUEUE_READY 0x044 // V2 only
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060 // Read only
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064    // Write only
#define VIRTIO_MMIO_STATUS 0x070

// V2 64-bit queue addresses
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

// virtio-blk request type
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

// Bits in desc.flags inside the queue
#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2

// --- Struct definitions (with padding) ---

struct virtq_desc {
  uint64_t addr;
  uint32_t len;
  uint16_t flags;
  uint16_t next;
};

struct virtq_avail {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[8];
};

struct virtq_used_elem {
  uint32_t id;
  uint32_t len;
};

struct virtq_used {
  uint16_t flags;
  uint16_t idx;
  struct virtq_used_elem ring[8];
};

struct virtio_blk_req {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
};

// Key: force padding to satisfy V1 PFN page alignment requirements
struct virtq {
  struct virtq_desc desc[8]; // 128 bytes
  struct virtq_avail avail;  // 20 bytes
  // Padding: make 'used' start at offset 4096
  uint8_t pad[4096 - sizeof(struct virtq_desc) * 8 - sizeof(struct virtq_avail)];
  struct virtq_used used; // aligned to 4096
} __attribute__((aligned(4096)));

// --- Global state ---

static volatile uint32_t *mmio = NULL;
// virtio-mmio version: 1 (legacy) or 2 (modern)
static uint32_t device_version = 0;
static struct virtq blk_virtq;
static struct virtio_blk_req blk_req;
static volatile uint8_t blk_status;

// Currently we submit only one request at a time, so we just track used.idx
static volatile uint16_t last_used_idx = 0;

void blk_init(void);
int blk_read_sector(uint64_t sector, void *buf);
int blk_write_sector(uint64_t sector, const void *buf);

#endif
