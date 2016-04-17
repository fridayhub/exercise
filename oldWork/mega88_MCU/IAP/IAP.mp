NOTE: AVR Code addresses are word addresses
All other addresses (including ones in FLASH) are byte addresses

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   00E4   0031 =     49. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       00E4  __idata_start
       0115  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   0116   04CC =   1228. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       008B  __text_start
       008B  __start
       00AD  _exit
       00AE  _wait_page_rw_ok
       00B9  _boot_page_ew
       00C4  _boot_page_fill
       00CD  _write_one_page
       0165  _uart_putchar
       016C  _uart_getchar
       017F  _uart_waitchar
       018A  _calcrc
       01B4  _quit
       01C8  _timer0_init
       01D3  _timer0_ovf_isr
       01FF  _port_init
       0207  _uart_init
       0217  _init_devices
       0231  _main
       02E2  push_arg4
       02E4  push_arg2
       02E7  push_xgset300C
       02EC  pop_xgset300C
       02F1  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   0200   0031 =     49. bytes (rel,con,ram)

       Addr  Global Symbol
      -----  --------------------------------
       0200  __data_start
       0200  _startupString
       0227  _address
       022B  _counter
       022D  _onesecond
       022F  _my_count
       0231  __data_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                             bss   0231   0080 =    128. bytes (rel,con,ram)

       Addr  Global Symbol
      -----  --------------------------------
       0231  __bss_start
       0231  _data
       02B1  __bss_end

Area                               Start  End    Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                          vector   0000   0044 =     68. bytes (abs,ovr,rom)

Files Linked      [ module(s) ]

C:\iccv7avr\lib\crtatmega.o	[ crtatmega.s ]
iap.o	[ iap.c ]
<library>	[ asave.s, gpushx.s ]

User Global Definitions

ram_end = 0x21ff
hwstk_size = 0x1e

User Base Address Definitions

func_lit = 0xe4
data = 0x200
eeprom:0.4096

