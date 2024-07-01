#include "epd7in5b_ext.h"

#include "epdif.h"


void DisplayFrame(unsigned char* image_data) {
  unsigned char temp1, temp2;
  epd.SendCommand(DATA_START_TRANSMISSION_1);

  for (long i = 0; i < 384; i++) {
    for (long j = 0; j < 160; j++) {
      temp1 = pgm_read_byte(image_data + i*160 + j);
      for (int k = 0; k < 2; k++) {
        if ((temp1 & 0xC0) == 0xC0) {
          temp2 = 0x03;                       // white
        } else if ((temp1 & 0xC0) == 0x00) {
          temp2 = 0x00;                       // black
        } else {
          temp2 = 0x04;                       // red
        }
        temp1 <<= 2;
        temp2 <<= 4;

        if((temp1 & 0xC0) == 0xC0) {
          temp2 |= 0x03;                      // white
        } else if ((temp1 & 0xC0) == 0x00) {
          temp2 |= 0x00;                      // black
        } else {
          temp2 |= 0x04;                      // red
        }
        temp1 <<= 2;
        epd.SendData(temp2);
      }
    }
  }

  epd.SendCommand(DISPLAY_REFRESH);
  epd.EpdIf::DelayMs(100);
  epd.WaitUntilIdle();
};

void DisplayQuarterFrame(unsigned char* image_data) {
  unsigned char temp1, temp2;
  epd.SendCommand(DATA_START_TRANSMISSION_1);

  for (int r = 0; r < 2; r++) {  // Row * 2
    for (long i = 0; i < 192; i++) {
      for (int c = 0; c < 2; c++) {  // Col * 2
        for (long j = 0; j < 80; j++) {
          temp1 = pgm_read_byte(image_data + i*80 + j);
          for (int k = 0; k < 2; k++) {
            if ((temp1 & 0xC0) == 0xC0) {
              temp2 = 0x03;                       // white
            } else if ((temp1 & 0xC0) == 0x00) {
              temp2 = 0x00;                       // black
            } else {
              temp2 = 0x04;                       // red
            }
            temp1 <<= 2;
            temp2 <<= 4;

            if((temp1 & 0xC0) == 0xC0) {
              temp2 |= 0x03;                      // white
            } else if ((temp1 & 0xC0) == 0x00) {
              temp2 |= 0x00;                      // black
            } else {
              temp2 |= 0x04;                      // red
            }
            temp1 <<= 2;
            epd.SendData(temp2); 
          }
        }
      }
    }
  }

  epd.SendCommand(DISPLAY_REFRESH);
  epd.EpdIf::DelayMs(100);
  epd.WaitUntilIdle();
};
