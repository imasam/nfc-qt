#include "nfchelper.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <nfc/nfc.h>
#include <QDebug>

extern "C"
{
#include "nfc-utils.h"
#include "mifare.h"
}

#define SAK_FLAG_ATS_SUPPORTED 0x20
#define CASCADE_BIT 0x04
#define MAX_FRAME_LEN 264
#define MAX_TARGET_COUNT 16

static int szRxBits;
static uint8_t abtRx[MAX_FRAME_LEN];

static nfc_target nt;
static mifare_param mp;
static mifare_classic_tag mtKeys;
static mifare_classic_tag mtDump;
static bool bUseKeyA;
static bool bUseKeyFile;
static bool bForceKeyFile;
static bool bTolerateFailures;
static bool magic2 = false;
static bool unlocked = false;
static uint8_t uiBlocks;

static const nfc_modulation nmMifare = {
    .nmt = NMT_ISO14443A,
    .nbr = NBR_106,
};

uint8_t abtHalt[4] = {0x50, 0x00, 0x00, 0x00};

// special unlock command
uint8_t abtUnlock1[1] = {0x40};
uint8_t abtUnlock2[1] = {0x43};

NfcHelper::NfcHelper()
{
    currentUid[0] = '\0';
}

bool NfcHelper::init()
{
    nfc_init(&context);
    if (context == NULL)
    {
        ERR("Unable to init libnfc (malloc)");
        return false;
    }

    // Try to open the NFC reader
    pnd = nfc_open(context, NULL);

    if (pnd == NULL)
    {
        ERR("Error opening NFC reader");
        nfc_exit(context);
        return false;
    }

    // Initialise NFC device as "initiator"
    if (nfc_initiator_init(pnd) < 0)
    {
        nfc_perror(pnd, "nfc_initiator_init");
        close();
        return false;
    }

    return true;
}

void NfcHelper::close()
{
    nfc_close(pnd);
    nfc_exit(context);
}

bool NfcHelper::setUid(char uid[])
{
    static uint8_t abtRawUid[12];
    static uint8_t abtAtqa[2];
    static uint8_t abtSak;
    static uint8_t abtAts[MAX_FRAME_LEN];
    static uint8_t szAts = 0;
    static size_t szCL = 1; // Always start with Cascade Level 1 (CL1)

    // ISO14443A Anti-Collision Commands
    uint8_t abtReqa[1] = {0x26};
    uint8_t abtSelectAll[2] = {0x93, 0x20};
    uint8_t abtSelectTag[9] = {0x93, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t abtWrite[4] = {0xa0, 0x00, 0x5f, 0xb1};
    uint8_t abtData[18] = {0x01, 0x23, 0x45, 0x67, 0x00, 0x08, 0x04, 0x00, 0x46, 0x59, 0x25, 0x58, 0x49, 0x10, 0x23, 0x02, 0x23, 0xeb};

    unsigned int c;
    char tmp[3] = {0x00, 0x00, 0x00};

    if (strlen(uid) == 8)
    {
        for (int i = 0; i < 4; ++i)
        {
            memcpy(tmp, uid + i * 2, 2);
            sscanf(tmp, "%02x", &c);
            abtData[i] = (char)c;
        }
        abtData[4] = abtData[0] ^ abtData[1] ^ abtData[2] ^ abtData[3];
        iso14443a_crc_append(abtData, 16);
    }
    else
    {
        return false;
    }

    if (!init()) // Fail to init nfc
        return false;

    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        nfc_close(pnd);
        nfc_exit(context);
        return false;
    }
    // Use raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        close();
        return false;
    }
    // Disable 14443-4 autoswitching
    if (nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, false) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        close();
        return false;
    }

    printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

    // Send the 7 bits request command specified in ISO 14443A (0x26)
    if (!transmit_bits(abtReqa, 7))
    {
        printf("Error: No tag available\n");
        close();
        return false;
    }
    memcpy(abtAtqa, abtRx, 2);

    // Anti-collision
    transmit_bytes(abtSelectAll, 2);

    // Check answer
    if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0)
    {
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
    if (abtSak & CASCADE_BIT)
    {
        szCL = 2; //or more
        // Check answer
        if (abtRawUid[0] != 0x88)
        {
            printf("WARNING: Cascade bit set but CT != 0x88!\n");
        }
    }

    if (szCL == 2)
    {
        // We have to do the anti-collision for cascade level 2

        // Prepare CL2 commands
        abtSelectAll[0] = 0x95;

        // Anti-collision
        transmit_bytes(abtSelectAll, 2);

        // Check answer
        if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0)
        {
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
        if (abtSak & CASCADE_BIT)
        {
            szCL = 3;
            // Check answer
            if (abtRawUid[0] != 0x88)
            {
                printf("WARNING: Cascade bit set but CT != 0x88!\n");
            }
        }

        if (szCL == 3)
        {
            // We have to do the anti-collision for cascade level 3

            // Prepare and send CL3 AC-Command
            abtSelectAll[0] = 0x97;
            transmit_bytes(abtSelectAll, 2);

            // Check answer
            if ((abtRx[0] ^ abtRx[1] ^ abtRx[2] ^ abtRx[3] ^ abtRx[4]) != 0)
            {
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
    /*if (abtRx[0] & SAK_FLAG_ATS_SUPPORTED) {
        iso_ats_supported = true;
    }*/

    printf("\nFound tag with\n UID: ");
    switch (szCL)
    {
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
    if (szAts > 1)
    { // if = 1, it's not actual ATS but error code
        printf(" ATS: ");
        print_hex(abtAts, szAts);
    }
    printf("\n");

    // now reset UID
    iso14443a_crc_append(abtHalt, 2);
    transmit_bytes(abtHalt, 4);

    if (!transmit_bits(abtUnlock1, 7))
    {
        printf("Warning: Unlock command [1/2]: failed / not acknowledged.\n");
        close();
        return false;
    }
    else
    {
        if (transmit_bytes(abtUnlock2, 1))
        {
            printf("Card unlocked\n");
        }
        else
        {
            printf("Warning: Unlock command [2/2]: failed / not acknowledged.\n");
        }
    }

    transmit_bytes(abtWrite, 4);
    transmit_bytes(abtData, 18);

    close();
    return true;
}

bool NfcHelper::getNewCard(char uid[], int &uidLen)
{
    if (!init()) // Fail to init nfc
        return nullptr;

    int count;
    nfc_target targets[MAX_TARGET_COUNT];

    nfc_modulation nm;
    nm.nmt = NMT_ISO14443A;
    nm.nbr = NBR_106;

    bool ret = false;

    // Get All ISO14443A targets
    if ((count = nfc_initiator_list_passive_targets(pnd, nm, targets, MAX_TARGET_COUNT)) >= 0)
    {
        int i;
        char tmp[10];
        nfc_iso14443a_info *info;

        for (i = 0; i < count; i++)
        {
            info = &(targets[i].nti.nai);
            for (size_t j = 0, off = 0; j < info->szUidLen; j++)
            {
                off += sprintf(tmp + off, "%02x", *(info->abtUid + j));
            }

            if (strcasecmp(tmp, currentUid) != 0)
            {
                strcpy(uid, tmp);
                ret = true;
                uidLen = info->szUidLen * 2;
                break;
            }
        }
    }

    close();

    return ret;
}

void NfcHelper::setCurrentUid(char newUid[])
{
    strcpy(currentUid, newUid);
}

bool NfcHelper::transmit_bits(const uint8_t *pbtTx, const size_t szTxBits)
{
    // Show transmitted command
    printf("Sent bits:     ");
    print_hex_bits(pbtTx, szTxBits);
    // Transmit the bit frame command, we don't use the arbitrary parity feature
    if ((szRxBits = nfc_initiator_transceive_bits(pnd, pbtTx, szTxBits, NULL, abtRx, sizeof(abtRx), NULL)) < 0)
        return false;

    // Show received answer
    printf("Received bits: ");
    print_hex_bits(abtRx, szRxBits);
    // Succesful transfer
    return true;
}

bool NfcHelper::transmit_bytes(const uint8_t *pbtTx, const size_t szTx)
{
    // Show transmitted command
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

/*
 * mfclassic-BEGIN
 */

void NfcHelper::print_success_or_failure(bool bFailure, uint32_t *uiBlockCounter)
{
    printf("%c", (bFailure) ? 'x' : '.');
    if (uiBlockCounter && !bFailure)
        *uiBlockCounter += 1;
}

bool NfcHelper::is_first_block(uint32_t uiBlock)
{
    // Test if we are in the small or big sectors
    if (uiBlock < 128)
        return ((uiBlock) % 4 == 0);
    else
        return ((uiBlock) % 16 == 0);
}

bool NfcHelper::is_trailer_block(uint32_t uiBlock)
{
    // Test if we are in the small or big sectors
    if (uiBlock < 128)
        return ((uiBlock + 1) % 4 == 0);
    else
        return ((uiBlock + 1) % 16 == 0);
}

uint32_t NfcHelper::get_trailer_block(uint32_t uiFirstBlock)
{
    // Test if we are in the small or big sectors
    uint32_t trailer_block = 0;
    if (uiFirstBlock < 128)
    {
        trailer_block = uiFirstBlock + (3 - (uiFirstBlock % 4));
    }
    else
    {
        trailer_block = uiFirstBlock + (15 - (uiFirstBlock % 16));
    }
    return trailer_block;
}

bool NfcHelper::authenticate(uint32_t uiBlock)
{
    mifare_cmd mc;

    // Set the authentication information (uid)
    memcpy(mp.mpa.abtAuthUid, nt.nti.nai.abtUid + nt.nti.nai.szUidLen - 4, 4);

    // Should we use key A or B?
    mc = (bUseKeyA) ? MC_AUTH_A : MC_AUTH_B;

    // Key file authentication.
    if (bUseKeyFile)
    {

        // Locate the trailer (with the keys) used for this sector
        uint32_t uiTrailerBlock;
        uiTrailerBlock = get_trailer_block(uiBlock);

        // Extract the right key from dump file
        if (bUseKeyA)
            memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyA, sizeof(mp.mpa.abtKey));
        else
            memcpy(mp.mpa.abtKey, mtKeys.amb[uiTrailerBlock].mbt.abtKeyB, sizeof(mp.mpa.abtKey));

        // Try to authenticate for the current sector
        if (nfc_initiator_mifare_cmd(pnd, mc, uiBlock, &mp))
            return true;
    }

    return false;
}

bool NfcHelper::unlock_card(void)
{
    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, false) < 0)
    {
        nfc_perror(pnd, "nfc_configure");
        return false;
    }
    // Use raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0)
    {
        nfc_perror(pnd, "nfc_configure");
        return false;
    }

    iso14443a_crc_append(abtHalt, 2);
    transmit_bytes(abtHalt, 4);
    // now send unlock
    if (!transmit_bits(abtUnlock1, 7))
    {
        printf("Warning: Unlock command [1/2]: failed / not acknowledged.\n");
    }
    else
    {
        if (transmit_bytes(abtUnlock2, 1))
        {
            printf("Card unlocked\n");
            unlocked = true;
        }
        else
        {
            printf("Warning: Unlock command [2/2]: failed / not acknowledged.\n");
        }
    }

    // reset reader
    // Configure the CRC
    if (nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        return false;
    }
    // Switch off raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, true) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");
        return false;
    }
    return true;
}

int NfcHelper::get_rats(void)
{
    int res;
    uint8_t abtRats[2] = {0xe0, 0x50};
    // Use raw send/receive methods
    if (nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false) < 0)
    {
        nfc_perror(pnd, "nfc_configure");
        return -1;
    }
    res = nfc_initiator_transceive_bytes(pnd, abtRats, sizeof(abtRats), abtRx, sizeof(abtRx), 0);
    if (res > 0)
    {
        // ISO14443-4 card, turn RF field off/on to access ISO14443-3 again
        if (nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false) < 0)
        {
            nfc_perror(pnd, "nfc_configure");
            return -1;
        }
        if (nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true) < 0)
        {
            nfc_perror(pnd, "nfc_configure");
            return -1;
        }
    }
    // Reselect tag
    if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0)
    {
        printf("Error: tag disappeared\n");

        close();
        return false;
    }
    return res;
}

bool NfcHelper::read_card(int read_unlocked)
{
    int32_t iBlock;
    bool bFailure = false;
    uint32_t uiReadBlocks = 0;

    if (read_unlocked)
    {
        //If the user is attempting an unlocked read, but has a direct-write type magic card, they don't
        //need to use the R mode. We'll trigger a warning and let them proceed.
        if (magic2)
        {
            printf("Note: This card does not require an unlocked write (R) \n");
            read_unlocked = 0;
        }
        else
        {
            //If User has requested an unlocked read, but we're unable to unlock the card, we'll error out.
            if (!unlock_card())
            {
                return false;
            }
        }
    }

    printf("Reading out %d blocks |", uiBlocks + 1);
    // Read the card from end to begin
    for (iBlock = uiBlocks; iBlock >= 0; iBlock--)
    {
        // Authenticate everytime we reach a trailer block
        if (is_trailer_block(iBlock))
        {
            if (bFailure)
            {
                // When a failure occured we need to redo the anti-collision
                if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0)
                {
                    printf("!\nError: tag was removed\n");
                    return false;
                }
                bFailure = false;
            }

            fflush(stdout);

            // Try to authenticate for the current sector
            if (!read_unlocked && !authenticate(iBlock))
            {
                printf("!\nError: authentication failed for block 0x%02x\n", iBlock);
                return false;
            }
            // Try to read out the trailer
            if (nfc_initiator_mifare_cmd(pnd, MC_READ, iBlock, &mp))
            {
                if (read_unlocked)
                {
                    memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, sizeof(mtDump.amb[iBlock].mbd.abtData));
                }
                else
                {
                    // Copy the keys over from our key dump and store the retrieved access bits
                    memcpy(mtDump.amb[iBlock].mbt.abtKeyA, mtKeys.amb[iBlock].mbt.abtKeyA, sizeof(mtDump.amb[iBlock].mbt.abtKeyA));
                    memcpy(mtDump.amb[iBlock].mbt.abtAccessBits, mp.mpt.abtAccessBits, sizeof(mtDump.amb[iBlock].mbt.abtAccessBits));
                    memcpy(mtDump.amb[iBlock].mbt.abtKeyB, mtKeys.amb[iBlock].mbt.abtKeyB, sizeof(mtDump.amb[iBlock].mbt.abtKeyB));
                }
            }
            else
            {
                printf("!\nfailed to read trailer block 0x%02x\n", iBlock);
                bFailure = true;
            }
        }
        else
        {
            // Make sure a earlier readout did not fail
            if (!bFailure)
            {
                // Try to read out the data block
                if (nfc_initiator_mifare_cmd(pnd, MC_READ, iBlock, &mp))
                {
                    memcpy(mtDump.amb[iBlock].mbd.abtData, mp.mpd.abtData, sizeof(mtDump.amb[iBlock].mbd.abtData));
                }
                else
                {
                    printf("!\nError: unable to read block 0x%02x\n", iBlock);
                    bFailure = true;
                }
            }
        }
        // Show if the readout went well for each block
        print_success_or_failure(bFailure, &uiReadBlocks);
        if ((!bTolerateFailures) && bFailure)
            return false;
    }
    printf("|\n");
    printf("Done, %d of %d blocks read.\n", uiReadBlocks, uiBlocks + 1);
    fflush(stdout);

    return true;
}

bool NfcHelper::write_card(int write_block_zero)
{
    uint32_t uiBlock;
    bool bFailure = false;
    uint32_t uiWriteBlocks = 0;

    if (write_block_zero)
    {
        //If the user is attempting an unlocked write, but has a direct-write type magic card, they don't
        //need to use the W mode. We'll trigger a warning and let them proceed.
        if (magic2)
        {
            printf("Note: This card does not require an unlocked write (W) \n");
            write_block_zero = 0;
        }
        else
        {
            //If User has requested an unlocked write, but we're unable to unlock the card, we'll error out.
            if (!unlock_card())
            {
                return false;
            }
        }
    }

    printf("Writing %d blocks |", uiBlocks + 1);
    // Write the card from begin to end;
    for (uiBlock = 0; uiBlock <= uiBlocks; uiBlock++)
    {
        // Authenticate everytime we reach the first sector of a new block
        if (is_first_block(uiBlock))
        {
            if (bFailure)
            {
                // When a failure occured we need to redo the anti-collision
                if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) <= 0)
                {
                    printf("!\nError: tag was removed\n");
                    return false;
                }
                bFailure = false;
            }

            fflush(stdout);

            // Try to authenticate for the current sector
            if (!write_block_zero && !authenticate(uiBlock) && !bTolerateFailures)
            {
                printf("!\nError: authentication failed for block %02x\n", uiBlock);
                return false;
            }
        }

        if (is_trailer_block(uiBlock))
        {
            // Copy the keys over from our key dump and store the retrieved access bits
            memcpy(mp.mpt.abtKeyA, mtDump.amb[uiBlock].mbt.abtKeyA, sizeof(mp.mpt.abtKeyA));
            memcpy(mp.mpt.abtAccessBits, mtDump.amb[uiBlock].mbt.abtAccessBits, sizeof(mp.mpt.abtAccessBits));
            memcpy(mp.mpt.abtKeyB, mtDump.amb[uiBlock].mbt.abtKeyB, sizeof(mp.mpt.abtKeyB));

            // Try to write the trailer
            if (nfc_initiator_mifare_cmd(pnd, MC_WRITE, uiBlock, &mp) == false)
            {
                printf("failed to write trailer block %d \n", uiBlock);
                bFailure = true;
            }
        }
        else
        {
            // The first block 0x00 is read only, skip this
            if (uiBlock == 0 && !write_block_zero && !magic2)
                continue;

            // Make sure a earlier write did not fail
            if (!bFailure)
            {
                // Try to write the data block
                memcpy(mp.mpd.abtData, mtDump.amb[uiBlock].mbd.abtData, sizeof(mp.mpd.abtData));
                // do not write a block 0 with incorrect BCC - card will be made invalid!
                if (uiBlock == 0)
                {
                    if ((mp.mpd.abtData[0] ^ mp.mpd.abtData[1] ^ mp.mpd.abtData[2] ^ mp.mpd.abtData[3] ^ mp.mpd.abtData[4]) != 0x00 && !magic2)
                    {
                        printf("!\nError: incorrect BCC in MFD file!\n");
                        printf("Expecting BCC=%02X\n", mp.mpd.abtData[0] ^ mp.mpd.abtData[1] ^ mp.mpd.abtData[2] ^ mp.mpd.abtData[3]);
                        return false;
                    }
                }
                if (!nfc_initiator_mifare_cmd(pnd, MC_WRITE, uiBlock, &mp))
                    bFailure = true;
            }
        }
        // Show if the write went well for each block
        print_success_or_failure(bFailure, &uiWriteBlocks);
        if ((!bTolerateFailures) && bFailure)
            return false;
    }
    printf("|\n");
    printf("Done, %d of %d blocks written.\n", uiWriteBlocks, uiBlocks + 1);
    fflush(stdout);

    return true;
}

typedef enum
{
    ACTION_READ,
    ACTION_WRITE,
    ACTION_USAGE
} action_t;

/*
 * Read or write(depend on command) the data of card to the cardDataFile
 * command: r|R: Read data from the card to the cardDataFile
 *          w|W: Write data from the cardDataFile to the card
 */
bool NfcHelper::mfclassic(const char *command, const char *cardDataFile)
{
    action_t atAction = ACTION_USAGE;
    uint8_t *pbtUID;
    uint8_t _tag_uid[4];
    uint8_t *tag_uid = _tag_uid;

    int unlock = 1;

    if (strcmp(command, "r") == 0 || strcmp(command, "R") == 0)
    {
        atAction = ACTION_READ;
    }
    else if (strcmp(command, "w") == 0 || strcmp(command, "W") == 0)
    {
        atAction = ACTION_WRITE;
    }

    bUseKeyA = true;
    bTolerateFailures = false;
    bUseKeyFile = true;
    bForceKeyFile = true;
    tag_uid = NULL;

    if (atAction == ACTION_USAGE)
    {
        return false;
    }

    // We don't know yet the card size so let's read only the UID from the keyfile for the moment
    if (bUseKeyFile)
    {
        FILE *pfKeys = fopen(cardDataFile, "rb");
        if (pfKeys == NULL)
        {
            printf("Could not open keys file: %s\n", cardDataFile);
            return false;
        }
        if (fread(&mtKeys, 1, 4, pfKeys) != 4)
        {
            printf("Could not read UID from key file: %s\n", cardDataFile);
            fclose(pfKeys);
            return false;
        }
        fclose(pfKeys);
    }
    
    if (!init())    // Fail to init nfc
        return false;

    // Let the reader only try once to find a tag
    if (nfc_device_set_property_bool(pnd, NP_INFINITE_SELECT, false) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");

        close();
        return false;
    }
    // Disable ISO14443-4 switching in order to read devices that emulate Mifare Classic with ISO14443-4 compliance.
    if (nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, false) < 0)
    {
        nfc_perror(pnd, "nfc_device_set_property_bool");

        close();
        return false;
    }

    printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));

    // Try to find a MIFARE Classic tag
    int tags;

    tags = nfc_initiator_select_passive_target(pnd, nmMifare, tag_uid, tag_uid == NULL ? 0 : 4, &nt);
    if (tags <= 0)
    {
        printf("Error: no tag was found\n");

        close();
        return false;
    }
    // Test if we are dealing with a MIFARE compatible tag
    if ((nt.nti.nai.btSak & 0x08) == 0)
    {
        printf("Warning: tag is probably not a MFC!\n");
    }

    // Get the info from the current tag
    pbtUID = nt.nti.nai.abtUid;

    if (bUseKeyFile)
    {
        uint8_t fileUid[4];
        memcpy(fileUid, mtKeys.amb[0].mbm.abtUID, 4);
        // Compare if key dump UID is the same as the current tag UID, at least for the first 4 bytes
        if (memcmp(pbtUID, fileUid, 4) != 0)
        {
            printf("Expected MIFARE Classic card with UID starting as: %02x%02x%02x%02x\n",
                   fileUid[0], fileUid[1], fileUid[2], fileUid[3]);
            printf("Got card with UID starting as:                     %02x%02x%02x%02x\n",
                   pbtUID[0], pbtUID[1], pbtUID[2], pbtUID[3]);
            if (!bForceKeyFile)
            {
                printf("Aborting!\n");

                close();
                return false;
            }
        }
    }
    printf("Found MIFARE Classic card:\n");
    print_nfc_target(&nt, false);

    // Guessing size
    uiBlocks = 0x3f;

    // Testing RATS
    int res;
    if ((res = get_rats()) > 0)
    {
        if ((res >= 10) && (abtRx[5] == 0xc1) && (abtRx[6] == 0x05) && (abtRx[7] == 0x2f) && (abtRx[8] == 0x2f) && ((nt.nti.nai.abtAtqa[1] & 0x02) == 0x00))
        {
            // MIFARE Plus 2K
            uiBlocks = 0x7f;
        }
        // Chinese magic emulation card, ATS=0978009102:dabc1910
        if ((res == 9) && (abtRx[5] == 0xda) && (abtRx[6] == 0xbc) && (abtRx[7] == 0x19) && (abtRx[8] == 0x10))
        {
            magic2 = true;
        }
    }
    printf("Guessing size: seems to be a %lu-byte card\n", (unsigned long)((uiBlocks + 1) * sizeof(mifare_classic_block)));

    if (bUseKeyFile)
    {
        FILE *pfKeys = fopen(cardDataFile, "rb");
        if (pfKeys == NULL)
        {
            printf("Could not open keys file: %s\n", cardDataFile);
            return false;
        }
        if (fread(&mtKeys, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfKeys) != (uiBlocks + 1) * sizeof(mifare_classic_block))
        {
            printf("Could not read keys file: %s\n", cardDataFile);
            fclose(pfKeys);
            return false;
        }
        fclose(pfKeys);
    }

    if (atAction == ACTION_READ)
    {
        memset(&mtDump, 0x00, sizeof(mtDump));
    }
    else
    {
        FILE *pfDump = fopen(cardDataFile, "rb");

        if (pfDump == NULL)
        {
            printf("Could not open dump file: %s\n", cardDataFile);
            return false;
        }

        if (fread(&mtDump, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfDump) != (uiBlocks + 1) * sizeof(mifare_classic_block))
        {
            printf("Could not read dump file: %s\n", cardDataFile);
            fclose(pfDump);
            return false;
        }
        fclose(pfDump);
    }
    // printf("Successfully opened required files\n");

    if (atAction == ACTION_READ)
    {
        if (read_card(unlock))
        {
            printf("Writing data to file: %s ...", cardDataFile);
            fflush(stdout);
            FILE *pfDump = fopen(cardDataFile, "wb");
            if (pfDump == NULL)
            {
                printf("Could not open dump file: %s\n", cardDataFile);

                close();
                return false;
            }
            if (fwrite(&mtDump, 1, (uiBlocks + 1) * sizeof(mifare_classic_block), pfDump) != ((uiBlocks + 1) * sizeof(mifare_classic_block)))
            {
                printf("\nCould not write to file: %s\n", cardDataFile);
                fclose(pfDump);

                close();
                return false;
            }
            printf("Done.\n");
            fclose(pfDump);
        }
    }
    else if (atAction == ACTION_WRITE)
    {
        write_card(unlock);
    }

    close();
    return true;
}

/*
 * mfclassic-END
 */
