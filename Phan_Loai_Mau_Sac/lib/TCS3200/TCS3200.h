#ifndef TCS3200_H
#define TCS3200_H

#include <Arduino.h>

class TCS3200 {
  public:
    // S0, S1: Tần số | S2, S3: Chọn màu | OUT: Đọc tín hiệu
    TCS3200(uint8_t S0, uint8_t S1, uint8_t S2, uint8_t S3, uint8_t OUT);
    
    void begin();
    void read();        // Đọc giá trị R, G, B
    String detect();    // Trả về: "RED", "BLUE", "YELLOW", "NONE"
    
    // Hàm debug để bạn in giá trị ra màn hình chỉnh ngưỡng
    void printRGB();    

  private:
    uint8_t _S0, _S1, _S2, _S3, _OUT;
    int R, G, B;
    
    int getPulse(uint8_t s2_state, uint8_t s3_state);
};

#endif