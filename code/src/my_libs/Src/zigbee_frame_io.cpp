#include "zigbee_frame_io.h"
#include "stdint.h"


#define TIMEOUT_MS 500

/* ===================== send 8-byte frame ===================== */
void SendFrame8(uint8_t type, uint8_t ctrl, uint8_t arg, uint8_t data)
{


    uint8_t tx[8];
    tx[0] = ZIGBEE_HEAD0;
    tx[1] = ZIGBEE_HEAD1;
    tx[2] = type;
    tx[3] = ctrl;
    tx[4] = arg;
    tx[5] = data;
    tx[6] = (tx[2] + tx[3] + tx[4] + tx[5]) & 0xFF;
    tx[7] = ZIGBEE_TAIL;

#if CURRENT_CAR == CAR_A
    Send_ZigbeeData_To_Fifo(tx, 8);
#elif CURRENT_CAR == CAR_B
    ExtSRAMInterface.ExMem_Write_Bytes(TX_DATA_ADDR, tx, 8);
#endif

}

/* ===================== low level read (align to 8-byte frame) ===================== */
bool ZigbeeRead8(uint8_t out8[8])
{
#if CURRENT_CAR == CAR_A
	  if (!out8) return false;

    uint32_t start = get_ms();

    while ((uint32_t)(get_ms() - start) < TIMEOUT_MS) {

        // Check if RX flag is set (frame received)
        if (Zigbee_Rx_flag == 1) {

            // Must keep this check (do not delete)
            if (gt_get_sub(canu_zibe_rxtime) == 0) {

                // Clear RX flag as early as possible
                Zigbee_Rx_flag = 0;

                // Validate frame header
                if (Zigb_Rx_Buf[0] == 0x55 && Zigb_Rx_Buf[1] == 0x02) {

                    // Copy 8 bytes out
                    for (int i = 0; i < 8; i++) {
                        out8[i] = Zigb_Rx_Buf[i];
                    }
                    return true; // Success
                }
            }
        }

        // Yield CPU a bit to avoid busy-wait burning cycles
        delay_ms(1);
    }

    return false; // Timeout

#elif CURRENT_CAR == CAR_B
    uint8_t flag = ExtSRAMInterface.ExMem_Read(RX_DATA_ADDR);
    if (flag == 0x00) return false;

    ExtSRAMInterface.ExMem_Read_Bytes(RX_DATA_ADDR, out8, 8);

    uint8_t zero[8] = {0};
    ExtSRAMInterface.ExMem_Write_Bytes(RX_DATA_ADDR, zero, 8);
    return true;
#endif
}
