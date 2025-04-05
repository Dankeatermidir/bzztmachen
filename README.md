YOU ARE USING THIS DEVICE AT YOUR OWN RISK, HIH VOLTAGES ARE DANGEROUS
I TAKE NO RESPONSIBILITY FOR ANYTHING THAT HAPPENS TO YOU.

#DESCRIPTION
Time to get electrocuted !
BzztMachen shocks you with high voltage square wave every time you take damage in game.
For this to work you'll need:
[BzztMachen Software]
[BzztMachen Hardware]
[BzztMachen Tables] for [Cheat engine], or make yourself a plugin idk

This code was made for esp32c3 supermini, but it should work on any esp32x development board with at least 2MB flash and 200kB ram.
Keep in mind you might need to make some minor changes in code if you use different board.

#INSTALLATION
Project is made using [platformio].
'''
#build project
pio run
#flash device
pio -t upload
