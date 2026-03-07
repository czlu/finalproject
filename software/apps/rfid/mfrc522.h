// MFRC522 RFID Reader Driver (SPI)
// For use with nRF52833 on micro:bit v2

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_spim.h"

// MFRC522 Registers
#define MFRC522_REG_COMMAND       0x01
#define MFRC522_REG_COM_I_EN      0x02
#define MFRC522_REG_DIV_I_EN      0x03
#define MFRC522_REG_COM_IRQ       0x04
#define MFRC522_REG_DIV_IRQ       0x05
#define MFRC522_REG_ERROR         0x06
#define MFRC522_REG_STATUS1       0x07
#define MFRC522_REG_STATUS2       0x08
#define MFRC522_REG_FIFO_DATA     0x09
#define MFRC522_REG_FIFO_LEVEL    0x0A
#define MFRC522_REG_WATER_LEVEL   0x0B
#define MFRC522_REG_CONTROL       0x0C
#define MFRC522_REG_BIT_FRAMING   0x0D
#define MFRC522_REG_COLL          0x0E
#define MFRC522_REG_MODE          0x11
#define MFRC522_REG_TX_MODE       0x12
#define MFRC522_REG_RX_MODE       0x13
#define MFRC522_REG_TX_CONTROL    0x14
#define MFRC522_REG_TX_ASK        0x15
#define MFRC522_REG_CRC_RESULT_H  0x21
#define MFRC522_REG_CRC_RESULT_L  0x22
#define MFRC522_REG_T_MODE        0x2A
#define MFRC522_REG_T_PRESCALER   0x2B
#define MFRC522_REG_T_RELOAD_H    0x2C
#define MFRC522_REG_T_RELOAD_L    0x2D
#define MFRC522_REG_AUTO_TEST     0x36
#define MFRC522_REG_VERSION       0x37

// MFRC522 Commands
#define MFRC522_CMD_IDLE          0x00
#define MFRC522_CMD_CALC_CRC      0x03
#define MFRC522_CMD_TRANSCEIVE    0x0C
#define MFRC522_CMD_MF_AUTHENT    0x0E
#define MFRC522_CMD_SOFT_RESET    0x0F

// PICC Commands
#define PICC_CMD_REQA             0x26
#define PICC_CMD_WUPA             0x52
#define PICC_CMD_ANTICOLL_CL1     0x93
#define PICC_CMD_SELECT_CL1       0x93

// Initialize the MFRC522 reader over SPI
// cs_pin: GPIO pin for chip select (e.g. EDGE_P16)
// rst_pin: GPIO pin for hardware reset (e.g. EDGE_P8)
void mfrc522_init(const nrfx_spim_t* spi, uint32_t cs_pin, uint32_t rst_pin);

// Check if a card is present (sends REQA)
bool mfrc522_is_card_present(void);

// Read the UID of a detected card
// uid_out: buffer to receive UID bytes (at least 4 bytes)
// Returns the number of UID bytes read, or 0 on failure
uint8_t mfrc522_read_uid(uint8_t* uid_out);

// Get a 32-bit card ID (convenience wrapper)
// Returns a non-zero ID if a card is detected, 0 otherwise
uint32_t mfrc522_get_id(void);
