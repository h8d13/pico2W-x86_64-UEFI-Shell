#include <string.h>
#include "tusb.h"
#include "iso_data.h"

#define CD_BLOCK_SIZE 2048

static uint32_t iso_block_count(void) {
    return (iso_image_len + CD_BLOCK_SIZE - 1) / CD_BLOCK_SIZE;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                        uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    const char vid[] = "PicoHack";
    const char pid[] = "BootCD-RP2350   ";
    const char rev[] = "1.0 ";
    memcpy(vendor_id, vid, 8);
    memcpy(product_id, pid, 16);
    memcpy(product_rev, rev, 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    (void) lun;
    *block_count = iso_block_count();
    *block_size  = CD_BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun; (void) power_condition; (void) start; (void) load_eject;
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void* buffer, uint32_t bufsize) {
    (void) lun;
    uint32_t addr = lba * CD_BLOCK_SIZE + offset;
    if (addr >= iso_image_len) return 0;
    uint32_t remaining = iso_image_len - addr;
    uint32_t to_copy = (bufsize < remaining) ? bufsize : remaining;
    memcpy(buffer, iso_image + addr, to_copy);
    if (to_copy < bufsize) {
        memset((uint8_t*)buffer + to_copy, 0, bufsize - to_copy);
    }
    return (int32_t) bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t* buffer, uint32_t bufsize) {
    (void) lun; (void) lba; (void) offset; (void) buffer; (void) bufsize;
    return -1;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return false;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                        void* buffer, uint16_t bufsize) {
    (void) lun;
    void const* response = NULL;
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        case 0x43: {  // READ TOC
            static uint8_t toc[20] = {
                0x00, 0x12,
                0x01, 0x01,
                0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x14, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00,
            };
            uint32_t blocks = iso_block_count();
            toc[16] = (blocks >> 24) & 0xFF;
            toc[17] = (blocks >> 16) & 0xFF;
            toc[18] = (blocks >> 8) & 0xFF;
            toc[19] = blocks & 0xFF;
            response = toc;
            resplen = sizeof(toc);
            break;
        }
        case 0x46: {  // GET CONFIGURATION
            static const uint8_t cfg[8] = {
                0x00, 0x00, 0x00, 0x04,
                0x00, 0x00,
                0x00, 0x08,  // current profile: CD-ROM
            };
            response = cfg;
            resplen = sizeof(cfg);
            break;
        }
        case 0x4A: {  // GET EVENT STATUS NOTIFICATION
            static const uint8_t evt[8] = {
                0x00, 0x06, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
            };
            response = evt;
            resplen = sizeof(evt);
            break;
        }
        case 0x51: {  // READ DISC INFORMATION
            static const uint8_t disc[34] = {
                0x00, 0x20,
                0x0E, 0x01, 0x01,
                0x01, 0x01,
                0x00,
                0x00, 0x00, 0x00, 0x00,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            };
            response = disc;
            resplen = sizeof(disc);
            break;
        }
        default:
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            return -1;
    }

    if (resplen > bufsize) resplen = bufsize;
    if (response && buffer && resplen > 0) memcpy(buffer, response, resplen);
    return resplen;
}
