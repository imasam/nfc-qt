#ifndef NFCHELPER_H
#define NFCHELPER_H

#include <stddef.h>
#include <stdint.h>

struct nfc_context;
struct nfc_device;

class NfcHelper
{
public:
    NfcHelper();
    bool setUid(char uid[9]);   // Set uid to card
    bool init();                // Init NFC
    void close();               // Close NFC
    bool transmit_bits(const uint8_t *pbtTx, const size_t szTxBits);
    bool transmit_bytes(const uint8_t *pbtTx, const size_t szTx);
    /*
     * Get a new card which is different from current
     */
    bool getNewCard(char uid[], int& uidLen);
    void setCurrentUid(char uid[9]);

    /*
     * Read or write(depend on command) the data of card to the cardDataFile
     */
    bool mfclassic(const char *command, const char *cardDataFile);

private:
    nfc_context *context;
    nfc_device *pnd;
    char currentUid[9];         // Uid of the current card

    static void print_success_or_failure(bool bFailure, uint32_t *uiBlockCounter);
    static bool is_first_block(uint32_t uiBlock);
    static bool is_trailer_block(uint32_t uiBlock);
    static uint32_t get_trailer_block(uint32_t uiFirstBlock);
    static bool authenticate(uint32_t uiBlock);
    static bool unlock_card(void);
    static int get_rats(void);
    static bool read_card(int read_unlocked);
    static bool write_card(int write_block_zero);

};

#endif // NFCHELPER_H
