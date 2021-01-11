# How to limit the Xiaomi Pro2 scooter's engine to 250watts

## Intro
This is written mostly for Dutch users so I will discuss the Dutch Law.

In order for this scooter (Xiaomi Pro2) to be legal in The Netherlands, the scooter must be recognized as an e-bike.

The law regarding e-bikes state:
* Maximum engine power 250w
* Maximum speed 25 kph
* User must provide own power in order to move (not only engine power)

source: [@MinIenW], [Rijksoverheid.nl]

The main problem with the Xiaomi Pro2 is that the engine has a nominal power of 300w and a peak power of 600w. In order to comply with Dutch we will limit the scooters engine to 250w nominal power. This will most times, but sometimes not give you to 25km/h depending on conditions.

## Tools used:
To do this I used an Android phone with an app called Xiaoflasher: </br> ![](assets/Tools1.png)

Using this app it will cost you €7,99 to unlock the app in full.
There are other free to use apps on several websites but this got a lot of positive feedback.

### Designing your own custom firmware:
Now that we have the app installed we are going to open the app and connect to the scooter.
1. Click on "Open Premium CFW Toolkit". </br> ![](assets/Tools2.png)
   1. Now you get to design your CFW for the scooter, there are a lot of functions. 
   2. We are only going to look at throttle and others.
2. Open tab throttle.
   1. We are going to limit the maximum current in Sport mode.
   2. Stock value is 25A, therefore I calculated that 25A resembles 300w, so 21A would be 252w.
   3. Select maximum current in sports mode and maximum speed.
      1. Set the current to 21A
      2. Max speed to 25km/h (I advise to select 26 maybe 27 it appears to be off by about 1 or 2km/h)
   4. Feel free to adjust the normal and pedestrian mode according to your needs.
   5. ![](assets/Tools3.png)
3. Deselect "motor start speed"
   1. The Ardiuno takes care of this
4. Turn of the brake light.
   1. Go to "Other Parameters" and select "Static brake light"
      1. This will deactivate the brake light and it will stop working completely.
      2. A brake light is not mandatory for e-bike's but if you have a brake light it must function properly. 
      3. Since the Xiaomi brake lights flash instead of being solid red, that's a fine according to the law. 
      4. You can leave it as is if you would prefer this is just a tip
   2. ![](assets/Tools4.png)
5. Press patch!
   1. The app will compile the software into a Zipp, all you have to do now is flash it to you scooter.
6. Go back to the start and look up your CFW zip.
   1. Press "Flash Zip".
   2. Wait until it is finished.
   3. Reboot the Scooter and you're done!
   4. ![](assets/Tools5.png)

## Other Tools:
* [Xiaomi Mijia M365 Custom Firmware Toolkit]
* [ScooterHacking Toolkit for Xiaomi Pro] (aka [m365pro.scooterhacking.org])
* All tools [ScooterHacking.org]

[@MinIenW]: https://twitter.com/MinIenW/status/1430900571209310208
[Rijksoverheid.nl]: https://www.rijksoverheid.nl/onderwerpen/fiets/vraag-en-antwoord/welke-regels-gelden-voor-mijn-elektrische-fiets-e-bike
[Xiaomi Mijia M365 Custom Firmware Toolkit]: https://m365.botox.bz
[scooterhacking.org]: https://scooterhacking.org
[m365pro.scooterhacking.org]: https://m365pro.scooterhacking.org
[ScooterHacking Toolkit for Xiaomi Pro]: https://pro.cfw.sh/
