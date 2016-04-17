NOTE: AVR Code addresses are word addresses
All other addresses (including ones in FLASH) are byte addresses

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                           idata   00E4   001B =     27. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       00E4  __idata_start
       00FF  __idata_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            text   0100   01F0 =    496. bytes (rel,con,rom)

       Addr  Global Symbol
      -----  --------------------------------
       0080  __text_start
       0080  __start
       00A2  _exit
       00A3  _uart_putchar
       00AA  _uart_getchar
       00BD  _uart_waitchar
       00C8  _quit
       00D8  _timer0_init
       00E3  _timer0_ovf_isr
       010F  _port_init
       0117  _uart_init
       0127  _init_devices
       0141  _main
       0178  __text_end

Area                               Addr   Size   Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                            data   0200   001B =     27. bytes (rel,con,ram)

       Addr  Global Symbol
      -----  --------------------------------
       0200  __data_start
       0200  _startupString
       0215  _counter
       0217  _onesecond
       0219  _my_count
       021B  __data_end

Area                               Start  End    Decimal Bytes (Attributes)
--------------------------------   ----   ----   ------- ----- ------------
                          vector   0000   0044 =     68. bytes (abs,ovr,rom)

Files Linked      [ module(s) ]

C:\iccv7avr\lib\crtatmega.o	[ crtatmega.s ]
Iap_app.o	[ Iap_app.c ]
<library>	

User Global Definitions

ram_end = 0x21ff
hwstk_size = 0x1e

User Base Address Definitions

func_lit = 0xe4
data = 0x200
eeprom:0.4096

