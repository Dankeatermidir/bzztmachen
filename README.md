YOU ARE USING THIS DEVICE AT YOUR OWN RISK, HIH VOLTAGES ARE DANGEROUS
I TAKE NO RESPONSIBILITY FOR ANYTHING THAT HAPPENS TO YOU.

#DESCRIPTION
Time to get electrocuted !
BzztMachen shocks you with high voltage square wave every time you take damage in game.
For this to work you'll need:
[BzztMachen Software](https://github.com/Dankeatermidir/bzztmachen)
[BzztMachen Hardware](https://github.com/Dankeatermidir/BzztMachenHardware)
[BzztMachen Tables](https://github.com/Dankeatermidir/BzztTables) for [Cheat engine](https://github.com/cheat-engine/cheat-engine), or make yourself a plugin idk

This code was made for esp32c3 supermini, but it should work on any esp32x development board with at least 2MB flash and 200kB ram.
Keep in mind you might need to make some minor changes in code if you use different board.

#INSTALLATION
Project is made using [platformio](https://platformio.org/platformio-ide) with [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) framework.
```
#build project
pio run
#flash device
pio -t upload
```
#HOW IT WORKS
First you gotta connect it to your local network, just use [ESP app](https://play.google.com/store/apps/details?id=com.espressif.provble)
Select "I don't have QR code" -> PROV_bzztmachen -> POP = imgonnagetzapped -> select network and enter password

Bzzt machen hosts http server with mdns so it's available at http://bzztmachen.local
It reacts to POST request in format {player}:{frequency}
Because impedance and stuff the higher frequency the lower is power.

Why HTTP?
bc including libraries in cheat engine is cancer, and http is available by default
