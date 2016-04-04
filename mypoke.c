#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>

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

void die(const char *reason) {
  perror(reason);
  exit(1);
}

int main(int argc, const char **argv) {
  int fd, i;
  struct ifreq ifr;
  struct ethtool_eeprom *eeprom;
  
  int offset = 2*(0x6870 + 0x8); /* PHY0 + MISC0 */
  int length = 2;
  int mod    = 0;
  int devid  = 0x1572; // Intel X710 DA4
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) die("socket");
  
  eeprom = calloc(1, sizeof(*eeprom) + length);
  if (!eeprom) die("calloc");
  
  for (i = 0; i < 4; ++i) {
    eeprom->cmd    = ETHTOOL_SEEPROM;
    eeprom->magic  = (devid << 16) | (I40E_NVM_SA << I40E_NVM_TRANS_SHIFT) | mod;
    eeprom->len    = length;
    eeprom->offset = offset + 0xc*i*2;
    
    *(uint16_t*)(eeprom+1) = 0x230c; // From 2b0c remove bit 0800 = qualification
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "eth3");
    ifr.ifr_data = (void*)eeprom;
    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("write");
    
    sleep(1);
  }
  
  // update checksum
  eeprom->cmd    = ETHTOOL_SEEPROM;
  eeprom->magic  = (devid << 16) | ((I40E_NVM_CSUM|I40E_NVM_SA) << I40E_NVM_TRANS_SHIFT) | mod;
  eeprom->len    = 2;
  eeprom->offset = 0;
  
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, "eth3");
  ifr.ifr_data = (void*)eeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("checksum");
  
  return 0;
}
