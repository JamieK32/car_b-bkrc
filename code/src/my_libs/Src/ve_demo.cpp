#include "vehicle_exchange.h"
#include "zigbee_frame_io.h"

#define LOG_VE_EN  1

#if LOG_VE_EN
  #define log_ve(fmt, ...)  LOG_P("[VE] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define log_ve(...)  do {} while(0)
#endif

void test_send_car_plate(void)
{
    ProtocolData_t data = {0};

    // Simulate car plate = "ABC123"
    data.car_plate[0] = 'A';
    data.car_plate[1] = 'B';
    data.car_plate[2] = 'C';
    data.car_plate[3] = '1';
    data.car_plate[4] = '2';
    data.car_plate[5] = '3';

    log_ve("Sending car plate: ABC123...\n");

    if (SendData(&data))
        log_ve("Data sent successfully.\n");
    else
        log_ve("Send failed!\n");
}

void test_receive_car_plate(void) {
    ProtocolData_t received = {0};

    log_ve("Waiting for car plate data...\n");

    if (WaitData(&received, 5000))
    {
        log_ve("Data received!\n");
        log_ve("Car plate: %c%c%c%c%c%c\n",
               received.car_plate[0],
               received.car_plate[1],
               received.car_plate[2],
               received.car_plate[3],
               received.car_plate[4],
               received.car_plate[5]);
    }
    else
    {
        log_ve("Timeout: no data received.\n");
    }

}

