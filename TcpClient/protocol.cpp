#include "protocol.h"


PDU *mkPDU(uint uiMsglen)
{
    uint uiPDULen = sizeof(PDU) + uiMsglen;
    PDU *pdu = (PDU*)malloc(uiPDULen);
    if(NULL == pdu)
    {
        exit(EXIT_FAILURE);
    }
    memset(pdu, 0, uiPDULen); //全设置为0
    pdu->uiPDULen = uiPDULen;
    pdu->uiMsgLen = uiMsglen;
    return pdu;
}
