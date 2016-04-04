#ifndef SYSCALLS_H

#define ETHTOOL_GEEPROM         0x0000000b /* Get EEPROM data */
#define ETHTOOL_SEEPROM         0x0000000c /* Set EEPROM data. */

#define I40E_NVM_TRANS_SHIFT    8
#define I40E_NVM_TRANS_MASK     (0xf << I40E_NVM_TRANS_SHIFT)
#define I40E_NVM_CON            0x0
#define I40E_NVM_SNT            0x1
#define I40E_NVM_LCB            0x2
#define I40E_NVM_SA             (I40E_NVM_SNT | I40E_NVM_LCB)
#define I40E_NVM_ERA            0x4
#define I40E_NVM_CSUM           0x8
#define I40E_NVM_EXEC           0xf

struct ethtool_eeprom {
  uint32_t cmd;
  uint32_t magic;
  uint32_t offset;
  uint32_t len;
  uint8_t  data[0];
};

#endif
