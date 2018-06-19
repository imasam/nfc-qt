#ifndef NFCHELPER_H
#define NFCHELPER_H

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

private:
    nfc_context *context;
    nfc_device *pnd;
    char currentUid[9];         // Uid of the current card

};

#endif // NFCHELPER_H
