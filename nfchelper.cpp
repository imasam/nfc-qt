#include "nfchelper.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
//#include <nfc/nfc.h>
#include "nfc-utils.h"

#define SAK_FLAG_ATS_SUPPORTED 0x20

#define MAX_FRAME_LEN 264


static uint8_t abtRx[MAX_FRAME_LEN];
static int szRxBits;
static uint8_t abtRawUid[12];
static uint8_t abtAtqa[2];
static uint8_t abtSak;
static uint8_t abtAts[MAX_FRAME_LEN];
static uint8_t szAts = 0;
static size_t szCL = 1;//Always start with Cascade Level 1 (CL1)
static nfc_device *pnd;

bool    iso_ats_supported = false;

// ISO14443A Anti-Collision Commands
uint8_t  abtReqa[1] = { 0x26 };
uint8_t  abtSelectAll[2] = { 0x93, 0x20 };
uint8_t  abtSelectTag[9] = { 0x93, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t  abtRats[4] = { 0xe0, 0x50, 0x00, 0x00 };
uint8_t  abtHalt[4] = { 0x50, 0x00, 0x00, 0x00 };
#define CASCADE_BIT 0x04

// special unlock command
uint8_t  abtUnlock1[1] = { 0x40 };
uint8_t  abtUnlock2[1] = { 0x43 };
uint8_t  abtWipe[1] = { 0x41 };
uint8_t abtWrite[4] = { 0xa0,  0x00,  0x5f,  0xb1 };
uint8_t abtData[18] = { 0x01,  0x23,  0x45,  0x67,  0x00,  0x08,  0x04,  0x00,  0x46,  0x59,  0x25,  0x58,  0x49,  0x10,  0x23,  0x02,  0x23,  0xeb };
uint8_t abtBlank[18] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x80, 0x69, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x36, 0xCC };


NfcHelper::NfcHelper()
{

}

static  bool transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
{
    // Show transmitted command\
    printf("Sent bits:     ");
    print_hex_bits(pbtTx, szTxBits);\
    // Transmit the bit frame command, we don't use the arbitrary parity feature
    if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, NULL, abtRx, sizeof(abtRx), NULL)) < 0)
        return false;

    // Show received answer\
    printf("Received bits: ");
    print_hex_bits(abtRx, szRxBits);\
    // Succesful transfer
    return true;
}


static  bool transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
    // Show transmitted command\
    printf("Sent bits:     ");
    print_hex(pbtTx, szTx);
    int res;
    // Transmit the command bytes
    if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, abtRx, sizeof(abtRx), 0)) < 0)
        return false;

    // Show received answer
    printf("Received bits: ");
    print_hex(abtRx, res);
    // Succesful transfer
    return true;
}

bool NfcHelper::setUid(char uid[])
{
    bool     format = false;
    unsigned int c;
    char     tmp[3] = { 0x00, 0x00, 0x00 };


    if (strlen(uid) == 8) {
        for (i = 0 ; i < 4 ; ++i) {
            memcpy(tmp, argv[arg] + i * 2, 2);
            sscanf(tmp, "%02x", &c);
            abtData[i] = (char) c;
        }
        abtData[4] = abtData[0] ^ abtData[1] ^ abtData[2] ^ abtData[3];
        iso14443a_crc_append(abtData, 16);
    }
    else {
        return false;
    }

    nfc_context *context;
    nfc_init(&context);
    if (context == NULL) {
        ERR("Unable to init libnfc (malloc)");
        return false;
    }

    // Try to open the NFC reader
    pnd = nfc_open(context, NULL);

    if (pnd == NULL) {
        ERR("Error opening NFC reader");
        nfc_exit(context);
        return false;
    }

    // Initialise NFC device as "initiator"
    if (nfc_initiator_init(pnd) < 0) {
        nfc_perror(pnd, "nfc_initiator_init");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }

    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0) {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }
    // Use raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0) {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }
    // Disable 14443-4 autoswitching
    if (nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, false) < 0) {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }

    printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

    // Send the 7 bits request command specified in ISO 14443A (0x26)
    if (!transmit_bits(abtReqa, 7)) {
        printf("Error: No tag available\n");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }
    memcpy(abtAtqa, abtRx, 2);

    // Anti-collision
    transmit_bytes(abtSelectAll, 2);

    // Check answer
    if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0) {
        printf("WARNING: BCC check failed!\n");
    }

    // Save the UID CL1
    memcpy(abtRawUid, abtRx, 4);

    //Prepare and send CL1 Select-Command
    memcpy(abtSelectTag + 2, abtRx, 5);
    iso14443a_crc_append(abtSelectTag, 7);
    transmit_bytes(abtSelectTag, 9);
    abtSak = abtRx[0];

    // Test if we are dealing with a CL2
    if (abtSak & CASCADE_BIT) {
        szCL = 2;//or more
        // Check answer
        if (abtRawUid[0] != 0x88) {
            printf("WARNING: Cascade bit set but CT != 0x88!\n");
        }
    }

    if (szCL == 2) {
        // We have to do the anti-collision for cascade level 2

        // Prepare CL2 commands
        abtSelectAll[0] = 0x95;

        // Anti-collision
        transmit_bytes(abtSelectAll, 2);

        // Check answer
        if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0) {
            printf("WARNING: BCC check failed!\n");
        }

        // Save UID CL2
        memcpy(abtRawUid + 4, abtRx, 4);

        // Selection
        abtSelectTag[0] = 0x95;
        memcpy(abtSelectTag + 2, abtRx, 5);
        iso14443a_crc_append(abtSelectTag, 7);
        transmit_bytes(abtSelectTag, 9);
        abtSak = abtRx[0];

        // Test if we are dealing with a CL3
        if (abtSak & CASCADE_BIT) {
            szCL = 3;
            // Check answer
            if (abtRawUid[0] != 0x88) {
                printf("WARNING: Cascade bit set but CT != 0x88!\n");
            }
        }

        if (szCL == 3) {
            // We have to do the anti-collision for cascade level 3

            // Prepare and send CL3 AC-Command
            abtSelectAll[0] = 0x97;
            transmit_bytes(abtSelectAll, 2);

            // Check answer
            if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0) {
                printf("WARNING: BCC check failed!\n");
            }

            // Save UID CL3
            memcpy(abtRawUid + 8, abtRx, 4);

            // Prepare and send final Select-Command
            abtSelectTag[0] = 0x97;
            memcpy(abtSelectTag + 2, abtRx, 5);
            iso14443a_crc_append(abtSelectTag, 7);
            transmit_bytes(abtSelectTag, 9);
            abtSak = abtRx[0];
        }
    }

    // Request ATS, this only applies to tags that support ISO 14443A-4
    if (abtRx[0] & SAK_FLAG_ATS_SUPPORTED) {
        iso_ats_supported = true;
    }

    printf("\nFound tag with\n UID: ");
    switch (szCL) {
    case 1:
        printf("%02x%02x%02x%02x", abtRawUid[0], abtRawUid[1], abtRawUid[2], abtRawUid[3]);
        break;
    case 2:
        printf("%02x%02x%02x", abtRawUid[1], abtRawUid[2], abtRawUid[3]);
        printf("%02x%02x%02x%02x", abtRawUid[4], abtRawUid[5], abtRawUid[6], abtRawUid[7]);
        break;
    case 3:
        printf("%02x%02x%02x", abtRawUid[1], abtRawUid[2], abtRawUid[3]);
        printf("%02x%02x%02x", abtRawUid[5], abtRawUid[6], abtRawUid[7]);
        printf("%02x%02x%02x%02x", abtRawUid[8], abtRawUid[9], abtRawUid[10], abtRawUid[11]);
        break;
    }
    printf("\n");
    printf("ATQA: %02x%02x\n SAK: %02x\n", abtAtqa[1], abtAtqa[0], abtSak);
    if (szAts > 1) { // if = 1, it's not actual ATS but error code
        printf(" ATS: ");
        print_hex(abtAts, szAts);
    }
    printf("\n");

    // now reset UID
    iso14443a_crc_append(abtHalt, 2);
    transmit_bytes(abtHalt, 4);

    if (!transmit_bits(abtUnlock1, 7)) {
        printf("Warning: Unlock command [1/2]: failed / not acknowledged.\n");
    } else {
        if (format) {
            transmit_bytes(abtWipe, 1);
            transmit_bytes(abtHalt, 4);
            transmit_bits(abtUnlock1, 7);
        }

        if (transmit_bytes(abtUnlock2, 1)) {
            printf("Card unlocked\n");
        } else {
            printf("Warning: Unlock command [2/2]: failed / not acknowledged.\n");
        }
    }

    transmit_bytes(abtWrite, 4);
    transmit_bytes(abtData, 18);
    if (format) {
        for (i = 3 ; i < 64 ; i += 4) {
            abtWrite[1] = (char) i;
            iso14443a_crc_append(abtWrite, 2);
            transmit_bytes(abtWrite, 4);
            transmit_bytes(abtBlank, 18);
        }
    }

    nfc_close(pnd);
    nfc_exit(context);
    return true;
}
