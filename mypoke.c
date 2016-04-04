#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include "syscalls.h"

void die(const char *reason) {
  perror(reason);
  exit(1);
}

int main(int argc, const char **argv) {
  int fd, i;
  struct ifreq ifr;
  struct ethtool_eeprom *eeprom;
  
  /* Run: lspci -n; Example:
    01:00.0 0200: 8086:1572 (rev 01)
    01:00.1 0200: 8086:1572 (rev 01)
    01:00.2 0200: 8086:1572 (rev 01)
    01:00.3 0200: 8086:1572 (rev 01)
                       ^^^^ find this field
   */
  int devid  = FILL_ME_IN; // 0x1572 = Intel X710 DA4
  const char *ethDev = FILL_ME_IN; // "eth3" = If card is on eth3
  /* Find a record like this:
    00006870 + 00 => 000b	<= Start of PHY0 recod
    00006870 + 01 => 0022
    00006870 + 02 => 0083
    00006870 + 03 => 1871
    00006870 + 04 => 0000
    00006870 + 05 => 0000
    00006870 + 06 => 3303
    00006870 + 07 => 000b
    00006870 + 08 => 2b0c	<= Register with bit 11 = module qualification
    00006870 + 09 => 0a00
    00006870 + 0a => 0a1e
    00006870 + 0b => 0003 
   */
  int phy0_offset = FILL_ME_IN; // 0x6870 on my card; find it with mytool
  
  int offset = 2*(phy0_offset + 0x8); /* PHY0 + MISC0 */
  int length = 2;
  int mod    = 0;
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) die("socket");
  
  eeprom = calloc(1, sizeof(*eeprom) + length);
  if (!eeprom) die("calloc");
  
  for (i = 0; i < 4; ++i) {
    eeprom->cmd    = ETHTOOL_SEEPROM;
    eeprom->magic  = (devid << 16) | (I40E_NVM_SA << I40E_NVM_TRANS_SHIFT) | mod;
    eeprom->len    = length;
    eeprom->offset = offset + 0xc*i*2;
    
    /* Remove bit 0800 = qualification from whatever was in register 8 */
    *(uint16_t*)(eeprom+1) = FILL_ME_IN; // 0x230c = 0x2b0c - 0x8000
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ethDev);
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
  strcpy(ifr.ifr_name, ethDev);
  ifr.ifr_data = (void*)eeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("checksum");
  
  return 0;
}
