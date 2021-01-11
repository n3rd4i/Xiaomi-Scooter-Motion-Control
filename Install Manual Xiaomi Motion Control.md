# [EN version of Handleiding inbouwen Xiaomi Motion Control](Handleiding&#32;inbouwen&#32;Xiaomi&#32;Motion&#32;Control.docx)
**DISCLAIMER** THIS DOCUMENT IS DIRECT TRANSLATION FROM ORIGINAL DOCUMENT FROM WHICH LICENCE DOES NOT BELONG TO ME! ALL RIGHT ARE RESERVED WITH THE OWNERS OF THE ORIGINAL DOCUMENT!

Before the Arduino is built in, software must be put on Arduino Nano. 
The software that is included is only to test the Arduino hardware-wise and does not offer support above 14 km/h.
To load the software, the Arduino IDE is needed.

This can be downloaded here: [Arduino IDE]

[Arduino driver CH340] (Only needed for  Windows):

Firmware can be found here, download the INO-file of the firmware:
* https://github.com/PsychoMnts/Xiaomi-Scooter-Motion-Control
* https://github.com/jelzo/Xiaomi-Scooter-Motion-Control
* https://github.com/kearfy/Xiaomi-Scooter-Motion-Control

*Some  variations require extra libraries, readthe github page. 

## [Installing Additional Arduino Libraries | Arduino]
1. Connect the Arduino Nano to the computer with a mini-USB cable
2. Open de Arduino INO file
3. Install the necessary libraries
   1. ![](assets/Picture1.png)
4. Search for arduino timer and install it with the Install button
   1. ![](assets/Picture2.png)
5. Set the correct board on ArduinoNano, and set the processor to ATmega328p (Old Bootloader)
   1. ![](assets/Picture3.png)
6. For Port, set the communication port of the arduino
7. Code upload
   1. ![](assets/Picture4.png)
8. If the upload is successful, build in the Arduino with the step-by-step plan below.

## Installation Steps:
1. Remove the plastic cover from the speedometer. </br> This is stuck but can usually be pulled loose with your nails. </br> __If it is very tight, you can use a hair dryer to soften the glue.__
   1. ![](assets/Install1.png)
2. Unscrew the circuit board in the handlebars.
   1. ![](assets/Install2.png)
3. Disconnect the connector.
   1. ![](assets/Install3.png)
4. Cut the tie-wrap from the rubber cap (collor may differ)
   1. ![](assets/Install4.png)
5. Disconnect all connectors. </br> __Be careful, these connectors are very sensitive. </br> When these are damaged, malfunctions can occur later__
   1. ![](assets/Install5.png)
6. Remove the cap and the right side of the handlebars.
   1. ![](assets/Install6.png)
7. Slide the rubber off the handlebars
   1. ![](assets/Install7.png)
8. Loosen the throttle with an allen so that it can slide.
   1. ![](assets/Install8.png)
9. Remove the rubber cover cap.
   1. ![](assets/Install9.png)
10. Remove the cord from the throttle and slide the entire throttle off the scooter. </br> **This step can be tricky. </br> Make sure that the connector is properly oriented and then it can be carried exactly through the hole.**
    1. ![](assets/Install10.png)
11. Unplug the handlebars with the allen key that comes with the scooter
    1. ![](assets/Install11.png)
12. Pass the extension cable with branch through the handlebar
    1. ![](assets/Install12.png)
13. Connect the extension cable and replace the steering wheel. </br> <span style="color:red">**Please note that you do not pierce cables with the repositioning of the screws!**</span>
    1. ![](assets/Install13.png)
14. Connect the  JST plug to the  gas cable. </br> <span style="color:red">**Note: AZdelivery Arduino Nano's have the JST plug soldered to the Nano.**</span>
    1. ![](assets/Install14.png)
15. Tie the cables back together with the supplied tie-wrap.
    1. ![](assets/Install15.png)
16. Connect the dashboard and screw it in
    1. ![](assets/Install16.png)
    2. ![](assets/Install17.png)
17. Stick the plastic cover back on the scooter.
    1. ![](assets/Install18.png)

[Arduino IDE]: https://www.arduino.cc/en/software
[Arduino driver CH340]: http://www.wch.cn/downloads/file/65.html
[Installing Additional Arduino Libraries | Arduino]: https://www.arduino.cc/en/Guide/Libraries