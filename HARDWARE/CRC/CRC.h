#ifndef __CRC_H
#define __CRC_H

void InvertUint8(unsigned char *DesBuf, unsigned char *SrcBuf);
void InvertUint16(unsigned short *DesBuf, unsigned short *SrcBuf);
unsigned short CRC16_MODBUS(unsigned char *puchMsg, unsigned int usDataLen);

#endif


