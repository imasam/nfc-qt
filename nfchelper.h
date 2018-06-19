#ifndef NFCHELPER_H
#define NFCHELPER_H

class NfcHelper
{
public:
    NfcHelper();
    bool setUid(char uid[9]);   // set uid to card
    bool init();                // init NFC
    void close();               // close NFC
    bool transmit_bits(const uint8_t *pbtTx, const size_t szTxBits);
    bool transmit_bytes(const uint8_t *pbtTx, const size_t szTx);

private:
    nfc_context *context;
    nfc_device *pnd;

};

#endif // NFCHELPER_H
