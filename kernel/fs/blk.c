/*
 * MiniOS
 * Copyright (C) 2025 lrisguan <lrisguan@outlook.com>
 * 
 * This program is released under the terms of the GNU General Public License version 2(GPLv2).
 * See https://opensource.org/licenses/GPL-2.0 for more information.
 * 
 * Project homepage: https://github.com/lrisguan/MiniOS
 * Description: A scratch implemention of OS based on RISC-V
 */

// blk.c - VirtIO-BLK driver (supports MMIO version 1 & 2)

#include "blk.h"
#include "../include/log.h"

// --- Helper functions ---

static inline void mmio_write(uint32_t off, uint32_t val) {
  if (mmio)
    mmio[off >> 2] = val;
}

static inline uint32_t mmio_read(uint32_t off) {
  if (mmio)
    return mmio[off >> 2];
  return 0;
}

// --- Interrupt handler (exported for trap.c) ---

// Return 1 if this interrupt is handled by the block device, 0 otherwise.
int blk_intr(void) {
  if (!mmio)
    return 0;

  // 1. Read and check interrupt status
  uint32_t status = mmio_read(VIRTIO_MMIO_INTERRUPT_STATUS);
  if ((status & 0x3) == 0)
    return 0;

  // 2. Acknowledge interrupt (ACK)
  mmio_write(VIRTIO_MMIO_INTERRUPT_ACK, status & 0x3);

  // 3. Let blk_do_io() notice completion by polling used.idx
  __sync_synchronize();
  if (blk_virtq.used.idx != last_used_idx) {
    return 1;
  }

  return 0;
}

// --- IO operations (interrupt-driven) ---

static int blk_do_io(uint32_t type, uint64_t sector, void *buf) {
  if (!mmio)
    return -1;

  // Use spinlock to prevent concurrent request submissions from corrupting descriptors
  // acquire(&blk_lock);

  blk_req.type = type;
  blk_req.reserved = 0;
  blk_req.sector = sector;
  blk_status = 0xff;

  // 1. Prepare descriptors
  uintptr_t buf_pa = V2P(buf);
  uintptr_t req_pa = V2P(&blk_req);
  uintptr_t status_pa = V2P(&blk_status);

  // 1. Fill descriptor chain: req -> data -> status
  blk_virtq.desc[0].addr = (uint64_t)req_pa;
  blk_virtq.desc[0].len = sizeof(blk_req);
  blk_virtq.desc[0].flags = VRING_DESC_F_NEXT;
  blk_virtq.desc[0].next = 1;

  blk_virtq.desc[1].addr = (uint64_t)buf_pa;
  blk_virtq.desc[1].len = 512;
  blk_virtq.desc[1].flags =
      (type == VIRTIO_BLK_T_IN) ? (VRING_DESC_F_NEXT | VRING_DESC_F_WRITE) : VRING_DESC_F_NEXT;
  blk_virtq.desc[1].next = 2;

  blk_virtq.desc[2].addr = (uint64_t)status_pa;
  blk_virtq.desc[2].len = 1;
  blk_virtq.desc[2].flags = VRING_DESC_F_WRITE;
  blk_virtq.desc[2].next = 0;

  // 2. Put descriptor chain into avail ring
  uint16_t aidx = blk_virtq.avail.idx;
  blk_virtq.avail.ring[aidx % 8] = 0; // head desc index

  __sync_synchronize();
  blk_virtq.avail.idx = aidx + 1;
  __sync_synchronize();

  // 3. Notify device to start processing
  mmio_write(VIRTIO_MMIO_QUEUE_NOTIFY, 0);

  // 4. Poll used.idx until completion (works for both V1 and V2, independent of interrupts)
  uint16_t expect = last_used_idx + 1;
  for (;;) {
    __sync_synchronize();
    if (blk_virtq.used.idx >= expect)
      break;
  }
  last_used_idx = blk_virtq.used.idx;

  if (blk_status != 0) {
    printk(BLUE "[INFO]: \tblk: io error status=%d" RESET "\n", blk_status);
    return -1;
  }
  return 0;
}

// --- Initialization ---

void blk_init(void) {
  uint32_t device_id;
  int found = 0;

  INFO("blk: probing device...");

  // Scan MMIO bus for a virtio-blk device
  for (uintptr_t addr = VIRTIO_MMIO_START; addr < VIRTIO_MMIO_END; addr += VIRTIO_MMIO_STRIDE) {
    volatile uint32_t *ptr = (uint32_t *)addr;
    if (ptr[VIRTIO_MMIO_MAGIC_VALUE >> 2] != 0x74726976)
      continue; // magic does not match

    uint32_t ver = ptr[VIRTIO_MMIO_VERSION >> 2];
    device_id = ptr[VIRTIO_MMIO_DEVICE_ID >> 2];
    if (device_id != 2)
      continue; // not a block device

    // Do not force version to match compile-time config; choose V1/V2 path at runtime via MMIO
    mmio = ptr;
    device_version = ver;
    found = 1;
    int irq_num = (addr - 0x10000000) / 0x1000;
    printk(BLUE "[INFO]: \tblk: found virtio-blk at 0x%x (IRQ %d, ver=%d)" RESET "\n", addr,
           irq_num, ver);
    break;
  }

  if (!found) {
    INFO("blk: not found");
    return;
  }

  // 1. Reset
  mmio_write(VIRTIO_MMIO_STATUS, 0);

  // 2. ACK & Driver
  uint32_t status = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
  mmio_write(VIRTIO_MMIO_STATUS, status);

  // 3. Feature negotiation: just read and then declare that we do not use optional features
  mmio_write(VIRTIO_MMIO_DEVICE_FEATURES_SEL, 0);
  uint32_t host_features = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES);
  (void)host_features; // currently unused

  mmio_write(VIRTIO_MMIO_DRIVER_FEATURES_SEL, 0);
  mmio_write(VIRTIO_MMIO_DRIVER_FEATURES, 0); // do not enable any optional features

  status |= VIRTIO_STATUS_FEATURES_OK;
  mmio_write(VIRTIO_MMIO_STATUS, status);

  if (device_version == 2) {
    if (!(mmio_read(VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK)) {
      printk("blk: feature negotiation failed\n");
      return;
    }
  }

  // 4. Queue Setup
  if (device_version == 1) {
    // Legacy device must be told guest page size before setting QUEUE_PFN
    mmio_write(VIRTIO_MMIO_GUEST_PAGE_SIZE, 4096);
  }

  mmio_write(VIRTIO_MMIO_QUEUE_SEL, 0);
  uint32_t qmax = mmio_read(VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (qmax == 0)
    return;
  if (qmax > 8)
    qmax = 8;
  mmio_write(VIRTIO_MMIO_QUEUE_NUM, qmax);

  uintptr_t base_pa = V2P(&blk_virtq);
  uintptr_t avail_pa = V2P(&blk_virtq.avail);
  uintptr_t used_pa = V2P(&blk_virtq.used);

  if (device_version == 1) {
    // V1: PFN setup + Align(4096)
    mmio_write(VIRTIO_MMIO_QUEUE_ALIGN, 4096);
    mmio_write(VIRTIO_MMIO_QUEUE_PFN, (uint32_t)(base_pa >> 12));
  } else {
    // V2: 64-bit addresses
    mmio_write(VIRTIO_MMIO_QUEUE_DESC_LOW, (uint32_t)base_pa);
    mmio_write(VIRTIO_MMIO_QUEUE_DESC_HIGH, (uint32_t)(base_pa >> 32));
    mmio_write(VIRTIO_MMIO_QUEUE_AVAIL_LOW, (uint32_t)avail_pa);
    mmio_write(VIRTIO_MMIO_QUEUE_AVAIL_HIGH, (uint32_t)(avail_pa >> 32));
    mmio_write(VIRTIO_MMIO_QUEUE_USED_LOW, (uint32_t)used_pa);
    mmio_write(VIRTIO_MMIO_QUEUE_USED_HIGH, (uint32_t)(used_pa >> 32));

    mmio_write(VIRTIO_MMIO_QUEUE_READY, 1);
  }

  // 5. Driver OK
  status |= VIRTIO_STATUS_DRIVER_OK;
  mmio_write(VIRTIO_MMIO_STATUS, status);

  last_used_idx = 0;
  blk_virtq.avail.idx = 0;
  blk_virtq.used.idx = 0;

  printk(BLUE "[INFO]: \tblk: initialized (ver=%d)" RESET "\n", device_version);
}

int blk_read_sector(uint64_t sector, void *buf) { return blk_do_io(VIRTIO_BLK_T_IN, sector, buf); }
int blk_write_sector(uint64_t sector, const void *buf) {
  return blk_do_io(VIRTIO_BLK_T_OUT, sector, (void *)buf);
}
