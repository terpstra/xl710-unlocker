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
  int fd;
  struct ifreq ifr;
  struct ethtool_eeprom *eeprom;
  
  int i;
  int offset = 0;
  int length = 64;
  int mod    = 0;
  uint16_t* ptr;
  
  /* Run: lspci -n; Example:
    01:00.0 0200: 8086:1572 (rev 01)
    01:00.1 0200: 8086:1572 (rev 01)
    01:00.2 0200: 8086:1572 (rev 01)
    01:00.3 0200: 8086:1572 (rev 01)
                       ^^^^ find this field
   */
  int devid  = FILL_ME_IN; // 0x1572 = Intel X710 DA4
  const char *ethDev = FILL_ME_IN; // "eth3" = If card is on eth3
  
  if (argc >= 2) offset = strtol(argv[1], 0, 0)*2;
  if (argc >= 3) length = strtol(argv[2], 0, 0)*2;
  if (argc >= 4) mod    = strtol(argv[3], 0, 0) & 0xFF;
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1) die("socket");
  
  eeprom = calloc(1, sizeof(*eeprom) + length);
  if (!eeprom) die("calloc");
  eeprom->cmd    = ETHTOOL_GEEPROM;
  eeprom->magic  = (devid << 16) | (I40E_NVM_SA << I40E_NVM_TRANS_SHIFT) | mod;
  eeprom->len    = length;
  eeprom->offset = offset;
  
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, ethDev);
  ifr.ifr_data = (void*)eeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("ioctl");
  
  ptr = (uint16_t*)(eeprom+1);
  for (i = 0; i < length/2; ++i) {
    printf("%08x + %02x => %04x\n", offset/2, i, *ptr++);
  }
  
  return 0;
}
