// MFRC522 RFID Reader Driver (SPI)

#include <string.h>
#include <stdio.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "mfrc522.h"

static const nrfx_spim_t* spi_inst;
static uint32_t cs_pin_num;

static void cs_select(void) {
  nrf_gpio_pin_clear(cs_pin_num);
}

static void cs_deselect(void) {
  nrf_gpio_pin_set(cs_pin_num);
}

static void write_register(uint8_t reg, uint8_t value) {
  uint8_t tx[2];
  tx[0] = (reg << 1) & 0x7E;
  tx[1] = value;

  cs_select();
  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(tx, 2);
  nrfx_spim_xfer(spi_inst, &xfer, 0);
  cs_deselect();
}

static uint8_t read_register(uint8_t reg) {
  uint8_t tx[2];
  uint8_t rx[2];
  tx[0] = ((reg << 1) & 0x7E) | 0x80;
  tx[1] = 0x00;

  cs_select();
  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TRX(tx, 2, rx, 2);
  nrfx_spim_xfer(spi_inst, &xfer, 0);
  cs_deselect();

  return rx[1];
}

static void set_bit_mask(uint8_t reg, uint8_t mask) {
  uint8_t val = read_register(reg);
  write_register(reg, val | mask);
}

static void clear_bit_mask(uint8_t reg, uint8_t mask) {
  uint8_t val = read_register(reg);
  write_register(reg, val & ~mask);
}

static void antenna_on(void) {
  uint8_t val = read_register(MFRC522_REG_TX_CONTROL);
  if ((val & 0x03) == 0) {
    set_bit_mask(MFRC522_REG_TX_CONTROL, 0x03);
  }
}

static bool communicate_with_picc(uint8_t command,
                                  const uint8_t* send_data, uint8_t send_len,
                                  uint8_t* recv_data, uint8_t* recv_len) {
  uint8_t irq_en = 0x00;
  uint8_t wait_irq = 0x00;

  if (command == MFRC522_CMD_TRANSCEIVE) {
    irq_en = 0x77;
    wait_irq = 0x30;
  }

  write_register(MFRC522_REG_COM_I_EN, irq_en | 0x80);
  clear_bit_mask(MFRC522_REG_COM_IRQ, 0x80);
  set_bit_mask(MFRC522_REG_FIFO_LEVEL, 0x80);
  write_register(MFRC522_REG_COMMAND, MFRC522_CMD_IDLE);

  for (uint8_t i = 0; i < send_len; i++) {
    write_register(MFRC522_REG_FIFO_DATA, send_data[i]);
  }

  write_register(MFRC522_REG_COMMAND, command);

  if (command == MFRC522_CMD_TRANSCEIVE) {
    set_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);
  }

  uint16_t timeout = 2000;
  uint8_t irq;
  do {
    irq = read_register(MFRC522_REG_COM_IRQ);
    timeout--;
  } while (timeout > 0 && !(irq & wait_irq) && !(irq & 0x01));

  clear_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);

  if (timeout == 0) {
    return false;
  }

  uint8_t error = read_register(MFRC522_REG_ERROR);
  if (error & 0x13) {
    return false;
  }

  if (recv_data && recv_len) {
    uint8_t fifo_len = read_register(MFRC522_REG_FIFO_LEVEL);
    if (fifo_len > *recv_len) {
      fifo_len = *recv_len;
    }
    *recv_len = fifo_len;
    for (uint8_t i = 0; i < fifo_len; i++) {
      recv_data[i] = read_register(MFRC522_REG_FIFO_DATA);
    }
  }

  return true;
}

void mfrc522_init(const nrfx_spim_t* spi, uint32_t cs_pin, uint32_t rst_pin) {
  spi_inst = spi;
  cs_pin_num = cs_pin;

  nrf_gpio_cfg_output(cs_pin_num);

  // Hardware reset with CS held LOW to select SPI mode
  // (if CS/SDA is HIGH during reset, module enters I2C mode)
  nrf_gpio_cfg_output(rst_pin);
  cs_select();                    // CS LOW during reset = SPI mode
  nrf_gpio_pin_clear(rst_pin);
  nrf_delay_ms(50);
  nrf_gpio_pin_set(rst_pin);
  nrf_delay_ms(50);
  cs_deselect();
  nrf_delay_ms(50);

  write_register(MFRC522_REG_T_MODE, 0x8D);
  write_register(MFRC522_REG_T_PRESCALER, 0x3E);
  write_register(MFRC522_REG_T_RELOAD_H, 0x00);
  write_register(MFRC522_REG_T_RELOAD_L, 0x1E);
  write_register(MFRC522_REG_TX_ASK, 0x40);
  write_register(MFRC522_REG_MODE, 0x3D);

  antenna_on();

  uint8_t ver = read_register(MFRC522_REG_VERSION);
  printf("MFRC522 version: 0x%02X\n", ver);
  printf("MFRC522 command reg: 0x%02X\n", read_register(MFRC522_REG_COMMAND));
  printf("MFRC522 status1 reg: 0x%02X\n", read_register(MFRC522_REG_STATUS1));
  printf("MFRC522 tx_control:  0x%02X\n", read_register(MFRC522_REG_TX_CONTROL));
  if (ver == 0x00 || ver == 0xFF) {
    printf("WARNING: Cannot communicate with MFRC522. Check SPI wiring!\n");
  }
}

bool mfrc522_is_card_present(void) {
  write_register(MFRC522_REG_BIT_FRAMING, 0x07);

  uint8_t cmd = PICC_CMD_REQA;
  uint8_t recv[2];
  uint8_t recv_len = sizeof(recv);

  bool result = communicate_with_picc(MFRC522_CMD_TRANSCEIVE, &cmd, 1, recv, &recv_len);
  return result && (recv_len == 2);
}

uint8_t mfrc522_read_uid(uint8_t* uid_out) {
  write_register(MFRC522_REG_BIT_FRAMING, 0x00);

  uint8_t cmd[2];
  cmd[0] = PICC_CMD_ANTICOLL_CL1;
  cmd[1] = 0x20;

  uint8_t recv[5];
  uint8_t recv_len = sizeof(recv);

  if (!communicate_with_picc(MFRC522_CMD_TRANSCEIVE, cmd, 2, recv, &recv_len)) {
    return 0;
  }

  if (recv_len != 5) {
    return 0;
  }

  uint8_t bcc = recv[0] ^ recv[1] ^ recv[2] ^ recv[3];
  if (bcc != recv[4]) {
    return 0;
  }

  memcpy(uid_out, recv, 4);
  return 4;
}

uint32_t mfrc522_get_id(void) {
  if (!mfrc522_is_card_present()) {
    return 0;
  }

  uint8_t uid[4];
  if (mfrc522_read_uid(uid) != 4) {
    return 0;
  }

  return ((uint32_t)uid[0] << 24) |
         ((uint32_t)uid[1] << 16) |
         ((uint32_t)uid[2] << 8)  |
         ((uint32_t)uid[3]);
}
