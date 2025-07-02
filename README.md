YOU ARE USING THIS DEVICE AT YOUR OWN RISK, HIH VOLTAGES ARE DANGEROUS
I TAKE NO RESPONSIBILITY FOR ANYTHING THAT HAPPENS TO YOU.

# DESCRIPTION
Time to get electrocuted !
BzztMachen shocks you with high voltage square wave every time you take damage in game.
For this to work you'll need:

1. [BzztMachen Software](https://github.com/Dankeatermidir/bzztmachen)
2. [BzztMachen Hardware](https://github.com/Dankeatermidir/BzztMachenHardware)
3. [BzztMachen Tables](https://github.com/Dankeatermidir/BzztTables) for [Cheat engine](https://github.com/cheat-engine/cheat-engine), or make yourself a plugin idk

This code was made for esp32c3 supermini, but it should work on any esp32x development board with at least 2MB flash and 200kB ram.
Keep in mind you might need to make some minor changes in code if you use different board.

# INSTALLATION
Project is made using [platformio](https://platformio.org/platformio-ide) with [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) framework.
```
#build project
pio run
#flash device
pio -t upload
```
# HOW IT WORKS
### Provisioning
First you gotta connect it to your local network, just use [ESP app](https://play.google.com/store/apps/details?id=com.espressif.provble)

Select "I don't have QR code" -> PROV_bzztmachen -> POP = imgonnagetzapped -> select network and enter password

### Requests
Bzzt machen hosts http server with mdns so it's available at http://bzztmachen.local/machen
It reacts to POST request in format {player}:{frequency}
Because impedance and stuff the higher frequency the lower is power.

Why HTTP?
bc including libraries in cheat engine is cancer, and http is available by default

### Reseting device
If you need to connect to new wi-fi network, you need to erase nvs, to do that you need to short RESET_PIN(10 by default) to ground for 1s, then you can go back to **provisioning** step.

# API
To trigger electric shock at given player, you need to specify **player_number** and **frequency** and make POST request to */machen* uri with given format:
"player_number,frequency", for example:
```
curl -d "0,50" http://bzztmachen.local:80/machen
```

**player_nymber** starts iterating from 0.

**frequency** is capped between 50 and 1000 by default. The higher frequency the lower power cuz impedance and stuff. 

**address** - bzztmachen uses mdns to avoid checking IP address every time, but it might be not resolved by mobile phones. On PC *bzztmachen.local* should be resolved without a problem.

### Return codes
*OK* - POST request succeded, electric shock was triggered.

*TOO SOON* - Given player was being shocked while request was made. No action taken.

*ERROR* - Request was probably in wrong format. No action taken.

### Light test
You can check if server is accessible an running without triggering shock by GET /version. It will return plain text answer with version and http methods.
```
curl bzztmachen.local/version
```
