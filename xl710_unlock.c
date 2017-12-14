#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>
#include "syscalls.h"


void die( const char *reason ) {
  perror( reason );
  exit( EXIT_FAILURE );
}

void print_usage( void )
{
  printf( "xl710_unlock\n" );
  printf( "  -n <device_name>, required\n" );
  printf( "  -i <device_id>, default: 0x1572\n" );
  printf( "  -p lock/unlock\n" );

  exit( EXIT_FAILURE );
}

int main(int argc, char *const *argv) {
  /* Parse arguments */
  char *c_devid = "0x1572";
  char *c_devname = NULL;
  int patching = 0;

  int c;

  while( ( c = getopt( argc, argv, "i:n:h?" ) ) != -1 )
  {
    switch( c )
    {
      case 'i':
        c_devid = optarg;
        break;
      case 'n':
        c_devname = optarg;
        break;
      case 'h':
      case '?':
      default:
        print_usage();
        break;
    }
  }

  if( c_devname == NULL ) print_usage();

  int mod = 0;
  uint16_t length = 0x02;
  int fd;
  struct ifreq ifr;
  struct ethtool_eeprom *eeprom;

  const int devid  = strtol( c_devid, 0, 0 );
  const char *ethDev = c_devname;

  fd = socket( AF_INET, SOCK_DGRAM, 0 );
  if( fd == -1 ) die( "socket" );

  eeprom = calloc( 1, sizeof( *eeprom ) + ( length << 1 ) );
  if( !eeprom ) die( "calloc" );

  eeprom->cmd    = ETHTOOL_GEEPROM;
  eeprom->magic  = (devid << 16) | (I40E_NVM_SA << I40E_NVM_TRANS_SHIFT) | mod;
  eeprom->len    = length;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, ethDev);

  /*
    Get offset to EMP SR
    offset 0x48
    length 0x2
  */
  eeprom->offset = 0x48 << 1;

  ifr.ifr_data = (void*)eeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("ioctl");

  uint16_t emp_offset = *(uint16_t*)(eeprom+1);
  printf("EMP SR offset: 0x%04x\n", emp_offset);

  /*
    Get offset to PHY Capabilities 0
    emp_offset + 0x19
    length 0x2
  */
  uint16_t cap_offset = 0x19;

  eeprom->offset = (emp_offset + cap_offset) << 1;

  ifr.ifr_data = (void*)eeprom;
  if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) die("ioctl");

  uint16_t phy_offset = *(uint16_t*)(eeprom+1) + emp_offset + cap_offset;
  printf("PHY offset: 0x%04x\n", phy_offset);

  /*
    Get PHY data size
    offset phy_offset
  */
  eeprom->offset = phy_offset << 1;

  ifr.ifr_data = (void*)eeprom;
  if( ioctl( fd, SIOCETHTOOL, &ifr ) == -1 ) die( "ioctl" );

  uint16_t phy_cap_size = *(uint16_t*)(eeprom + 1);
  printf("PHY data struct size: 0x%04x\n", phy_cap_size);

  /*
    Get misc0
  */

  uint16_t misc_offset = 0x8;

  int i;
  uint16_t misc0 = 0x0;
  int change_count = 0;

  for( i = 0; i < 4; ++i)
  {
    eeprom->offset = (phy_offset + misc_offset + (phy_cap_size + 1) * i) << 1;

    ifr.ifr_data = (void*)eeprom;
    if( ioctl( fd, SIOCETHTOOL, &ifr ) == -1 ) die( "ioctl" );

    uint16_t misc = *(uint16_t*)(eeprom + 1);
    printf( "MISC: 0x%04x", misc);

    if( misc & 0x0800 ) printf( " <- locked\n" );
    else printf( " <- unlocked\n" );

    if( misc != misc0 )
    {
      ++change_count;
      misc0 = misc;
    }
  }

  if( change_count > 1 ) die( "Different MISC's values" );

  /*
    Patching
  */

  printf( "Ready to fix it? [y/N]: " );
  char choice = getchar();
  switch( choice )
  {
    case 'y':
    case 'Y':
      patching = 1;
      break;
    default:
      patching = 0;
  }

  if( patching )
  {
    for( i = 0; i < 4; ++i)
    {
      eeprom->cmd = ETHTOOL_SEEPROM;
      eeprom->offset = (phy_offset + misc_offset + (phy_cap_size + 1) * i) << 1;

      *(uint16_t*)(eeprom + 1) = misc0 ^ 0x0800;
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
  }

  return 0;
}
