# xl710_patch

This program unlocks intel x710 NIC.

## Usage

* test

```shell
# xl710_patch -n enp4s0f0
EMP SR offset: 0x67a8
PHY offset: 0x68f6
PHY data struct size: 0x000c
MISC: 0x630c <- unlocked
MISC: 0x630c <- unlocked
MISC: 0x630c <- unlocked
MISC: 0x630c <- unlocked
```

* lock/unlock

```shell
# xl710_patch -n enp4s0f0 -p
...
```
