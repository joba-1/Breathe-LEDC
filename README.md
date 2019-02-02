# Breathe LEDC Example

Me trying to make an esp32 breathe a led with LEDC api and background task.

## Build - Flash - Monitor
* Prepare shell to use ESP-IDF (see my [Blink-ULP repo](https://github.com/joba-1/Blink-ULP/blob/master/README.md) for details)
```
. ~/esp32/env.sh
```

* Fetch and build this example, flash it and open serial console (adapt ESPPORT and ESPBAUD to your setup. Can be configured in sdkconfig file)
```
mkdir /tmp/breathe-$$ && \
cd /tmp/breathe-$$ && \
git clone https://github.com/joba-1/Breathe-LEDC.git && \
cd Breathe-LEDC && \
make -j8 flash monitor ESPPORT=/dev/ttyUSB0 ESPBAUD=115200

```
A led connected to gnd (kathode) and gpio27 (anode) should breathe (fade in and fade out) now.

Exit the monitor with [Ctrl]-] (which is [Strg]-[Alt-Gr]-] on german keyboard)

JoBa1
