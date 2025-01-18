# Anti-Deauth Wi-Fi Protection System

This project is an ESP32-based solution designed to detect and prevent deauthentication (deauth) attacks on Wi-Fi networks. It leverages ESP32's promiscuous mode to sniff for deauth packets and dynamically prevents Wi-Fi attacks by attempting to connect to a network using fake credentials. The system also scans for duplicate SSIDs and open networks, offering an additional layer of security.

## Features
- **Deauth Packet Detection**: Detects deauthentication (0xC0) packets in the network.
- **Fake Password Connection**: When a deauth attack is detected, the ESP32 attempts to connect to the network using a randomly generated fake password, confusing potential attackers.
- **Duplicate SSID Detection**: Scans nearby Wi-Fi networks for duplicate SSIDs and checks if any of them are open.
- **Promiscuous Mode**: Uses ESP32's promiscuous mode to monitor and capture Wi-Fi traffic for analysis.

#demo
![IMG_20250118_121628](https://github.com/user-attachments/assets/065bf3ff-b3aa-4f63-b942-fe553d10b199)
![IMG_20250118_121642](https://github.com/user-attachments/assets/0d3fb4bd-0f51-45c7-a240-df373e2c4902)
![IMG_20250118_121607](https://github.com/user-attachments/assets/989f9ce4-c3db-46f4-97f1-091d246f9db3)
![IMG_20250118_124240](https://github.com/user-attachments/assets/9c606937-3c58-459b-8c00-78aecaab073a)
![IMG_20250118_124253_1](https://github.com/user-attachments/assets/b544c906-45ba-4540-b5b8-339263691613)

#Web Interface
![Screenshot 2025-01-18 122630](https://github.com/user-attachments/assets/a93975f5-89cf-4640-9b1c-7373104592e3)
![Screenshot 2025-01-18 122558](https://github.com/user-attachments/assets/fab96612-80a2-4db3-ae57-52845adb661f)


## Hardware Requirements
- ESP32 development board
- Micro-USB cable
- Wi-Fi network (for testing)

## Software Requirements
- Arduino IDE (with ESP32 board support)
- ESP32 Wi-Fi libraries (`esp_wifi.h` and `WiFi.h`)

## Setup Instructions

### 1. Install Arduino IDE
Make sure you have the Arduino IDE installed on your machine. You can download it from [here](https://www.arduino.cc/en/software).

### 2. Install ESP32 Board Support
In the Arduino IDE:
1. Go to **File > Preferences**.
2. In the "Additional Boards Manager URLs" field, add the following URL:  
   `https://dl.espressif.com/dl/package_esp32_index.json`
3. Go to **Tools > Board > Board Manager**, search for `ESP32` and install the package.

### 3. Install Required Libraries
Ensure that the following libraries are installed in the Arduino IDE:
- `WiFi.h`
- `Wire.h`
- `esp_wifi.h`
- `WebServer.h`
- `WebSocketsServer.h`
- `esp_system.h`
- `esp_spi_flash.h`
- `ArduinoJson.h`
- `esp_heap_caps.h`
- `Adafruit_GFX.h`
- `Adafruit_SSD1306.h`

### 4. Wiring
- **LED**: The onboard LED (pin 2 on ESP32) will blink to indicate the system is running.

### 5. Upload the Code
1. Copy and paste the provided code into your Arduino IDE.
2. Replace `your_ssid` with the SSID of the Wi-Fi network you want to protect.
3. Select the appropriate ESP32 board from **Tools > Board**.
4. Connect the ESP32 to your computer and select the correct COM port under **Tools > Port**.
5. Click on the **Upload** button to upload the code to the ESP32.

## Code Breakdown

### 1. `setup()`
- Initializes the serial monitor and sets the ESP32 in Wi-Fi Station mode.
- Enables promiscuous mode to allow packet sniffing.
- Assigns a callback function (`sniffer_callback`) to handle detected packets.

### 2. `loop()`
- The onboard LED blinks every second to indicate that the system is running.
- The `checkDuplicateSSIDs()` function is called periodically to scan for duplicate SSIDs and detect open networks.

### 3. `generateRandomPassword()`
- Generates a random password using a set of alphanumeric characters and special symbols to confuse hackers when attempting to connect to a Wi-Fi network.

### 4. `prevent()`
- When a deauth attack is detected, this function attempts to connect to the network using a randomly generated fake password. This process helps mitigate attacks by keeping the attacker busy.

### 5. `sniffer_callback()`
- Listens for packets in promiscuous mode and checks if any packet is a deauthentication frame (0xC0).
- If a deauth packet is detected, it triggers the `prevent()` function to mitigate the attack.

### 6. `checkDuplicateSSIDs()`
- Scans nearby Wi-Fi networks and checks if any duplicate SSIDs exist.
- If a duplicate SSID is found and is open (i.e., no encryption), it triggers the `prevent()` function to avoid connecting to unsecured networks.

## How It Works
1. **Deauth Detection**: The ESP32 continuously monitors nearby Wi-Fi packets in promiscuous mode. If it detects a deauthentication packet (a common type of Wi-Fi attack), it triggers the defense mechanism.
2. **Fake Password Defense**: Upon detecting an attack, the ESP32 attempts to connect to the network using a randomly generated fake password. This confuses attackers by simulating network activity.
3. **Network Scanning**: The system periodically scans for duplicate SSIDs and open networks. If a duplicate SSID is found, it triggers the defense to avoid insecure connections.

## Potential Use Cases
- **Home and Small Business Wi-Fi Security**: Protect personal or business Wi-Fi networks from basic deauth attacks without needing expensive security equipment.
- **Educational Purpose**: Useful as a teaching tool to understand Wi-Fi vulnerabilities and how to defend against them.
- **Public Wi-Fi Protection**: Could be deployed in public Wi-Fi networks (e.g., cafes, airports) to monitor and mitigate deauth attacks.

## Limitations
- This project protects against basic deauth attacks but may not be effective against more sophisticated Wi-Fi attacks (e.g., WPA3 networks with PMF enabled).
- The ESP32 hardware has limited processing power, so it may not be suitable for high-traffic networks or enterprise-level security.

## License
This project is open-source and available under the MIT License.

Feel free to contribute and modify the code to improve its features and security capabilities!

---

### References
- [ESP32 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Wi-Fi Deauthentication Attacks Explained](https://null-byte.wonderhowto.com/how-to/hack-wi-fi-detect-deauthentication-attack-your-network-using-esp8266-0196686/)
