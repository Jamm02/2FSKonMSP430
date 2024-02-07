Implementation of 2FSK on MSP430FR5969:

- Import firmware_2fsk project in CCStudio to flash the FSK firmware.
- FSK output pin is P1.4.
- Packet_gen.c contains a 4 bit grey image data of size 100x100 pixels in hex format.
- After the image is sent the packet only contains 0xFFFF as payload.

Transmitting image over a carrier: 

- Grey_image_to_hex.ipynb gives out the required hex data of the image.
- recounst_image.ipynb reconstructs the image from the data received from CC1310(received_grey_image.txt).