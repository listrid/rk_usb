***
# RK_USB
The low level tools for rockchip SOC with maskrom and loader mode support.
 fork XROCK 

## How to build

### Linux platform

```shell
cd rk_usb
make
```

### Window platform

build project rk_usb.sln from Visual Studio




## Usage

```
usage:
    rk_usb maskrom <ddr> <usbplug>                - Initial chip using ddr and usbplug in maskrom mode
    rk_usb download <loader>                      - Initial chip using loader in maskrom mode
    rk_usb upgrade <loader>                       - Upgrade loader to flash in loader mode
    rk_usb ready                                  - Show chip ready or not
    rk_usb version                                - Show chip version
    rk_usb capability                             - Show capability information
    rk_usb reset [maskrom]                        - Reset chip to normal or maskrom mode
    rk_usb otp <length>                           - Dump otp memory in hex format
    rk_usb sn                                     - Read serial number
    rk_usb sn <string>                            - Write serial number
    rk_usb vs dump <index> <length> [type]        - Dump vendor storage in hex format
    rk_usb vs read <index> <length> <file> [type] - Read vendor storage
    rk_usb vs write <index> <file> [type]         - Write vendor storage
    rk_usb storage                                - Read storage media list
    rk_usb storage <index>                        - Switch storage media and show list
    rk_usb flash                                  - Detect flash and show information
    rk_usb flash erase <sector> <count>           - Erase flash sector
    rk_usb flash read <sector> <count> <file>     - Read flash sector to file
    rk_usb flash write <sector> <file>            - Write file to flash sector
extra:
    rk_usb extra maskrom [--rc4 on|off] [--sram <file> --delay <ms>] [--dram <file> --delay <ms>] [...]
    rk_usb extra dump-arm32 <uart_register> <address> <length>
    rk_usb extra dump-arm64 <uart_register> <address> <length>
    rk_usb extra write-arm32 <address> <file>
    rk_usb extra write-arm64 <address> <file>
    rk_usb extra exec-arm32 <address>
    rk_usb extra exec-arm64 <address>

[--rc4 on]  enable decryption file
```



## Tips

- The maskrom command can only used in maskrom mode, Before executing other commands, you must first execute the maskrom command

- The memory base address from 0, **NOT** sdram's physical address.

- In some u-boot rockusb driver, The flash dump operation be limited to the start of 32MB, you can patch u-boot's macro `RKUSB_READ_LIMIT_ADDR`.



### RK3588

```shell
rk_usb maskrom rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.16.bin rk3588_usbplug_v1.11.bin
rk_usb version
```

```shell\
rk_usb extra maskrom --sram rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.16.bin --delay 10 --dram rk3588_usbplug_v1.11.bin --delay 10
rk_usb version
```

- Initial ddr memory

```shell
rk_usb extra maskrom --sram rk3588_ddr_lp4_2112MHz_lp5_2400MHz_v1.16.bin --delay 10
```

- Dump memory region in hex format by debug uart

```shell
rk_usb extra maskrom-dump-arm64 --uart 0xfeb50000 0xffff0000 1024
```


### RK3506

```shell
rk_usb maskrom rk3506b_ddr_750MHz_v1.04.bin rk3506_usbplug_v1.02.bin
rk_usb version
```


```shell
rk_usb extra maskrom --sram rk3506_ddr_750MHz_v1.05.bin --delay 10 --dram rk3506_usbplug_v1.02.bin --delay 10
rk_usb version
```


- Initial ddr memory

```shell
rk_usb extra maskrom --sram rk3506_ddr_750MHz_v1.05.bin --delay 10
```


- Dump memory region in hex format by debug uart

```shell
rk_usb extra dump-arm32 0xff0a0000 0xff910000 1024
```


## Links
* [ XROCK ](https://github.com/xboot/xrock)
* [The rockchip loader binaries](https://github.com/rockchip-linux/rkbin)
* [The rockchip rkdeveloptool](https://github.com/rockchip-linux/rkdeveloptool)



## License

This library is free software; you can redistribute it and or modify it under the terms of the MIT license. See [MIT License](LICENSE) for details.
