#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_heap_caps.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Constants for the log box
#define LOG_BOX_X 5
#define LOG_BOX_Y 24
#define LOG_BOX_WIDTH 115
#define LOG_BOX_HEIGHT 35
#define LINE_SPACING 8 // Space between log lines

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
// Initialize OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// State variables
int currentScreen = 0; // 0 = WiFi screen, 1 = Screen 1, 2 = Screen 2
int cleanCount = 0;
// Data log buffer
String logEntries[20]; // Circular buffer for log entries (adjust size as needed)
int logIndex = 0;       // Circular buffer index
int visibleLogOffset = 0; // Index to manage scrolling of logs
int deauthTrying = 0;

// Replace with your network credentials
const char* ssid = "server";
const char* password = "@pritam123";

const int passwordLength = 8;
String charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+";

// Pin assignments
const int switch1Pin = 12;  // Switch 1 connected to D2
const int switch2Pin = 13;  // Switch 2 connected to D3

// Variables to store LED states
bool led1State = false;
bool led2State = false;

// Variables to debounce the switches
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 500; // Debounce delay in milliseconds

const int ledPin = 2;

// Create a WebServer object on port 80
WebServer server(80);              // Web server on port 80
WebSocketsServer webSocket(81);     // WebSocket server on port 81

// HTML code for the web page
const char* htmlPage = R"====(
<!DOCTYPE html>
<html lang="en" data-theme="light">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Compact Network Security Dashboard</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js"></script>
  <style>
    :root {
      --bg-color: #ffffff;
      --card-bg: #f0f0f0;
      --text-color: #333333;
      --accent-color: #3a86ff;
      --success-color: #4caf50;
      --warning-color: #ff9800;
      --danger-color: #f44336;
    }

    [data-theme="dark"] {
      --bg-color: #0f0f1a;
      --card-bg: #1a1a2e;
      --text-color: #e0e0e0;
    }

    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      user-select: none;
    }

    body {
      font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
      background-color: var(--bg-color);
      color: var(--text-color);
      line-height: 1.4;
      transition: background-color 0.3s, color 0.3s;
      font-size: 14px;
    }

    canvas {
      height: 100%;
      width: auto;
      margin: auto;
    }

    p {
      font-size: 1rem;
    }

    li {
      list-style: none;
    }

    ul {
      display: flex;
      padding: 6px;
      gap: 15px;
      align-items: center;
      justify-content: center;
    }

    h1 {
      margin: 0px;
      font-family: monospace;
    }

    svg {
      height: auto;
      width: 32px;
    }

    .nav {
      display: flex;
      background-color: #d6d6ff63;
      flex-direction: row;
      align-items: center;
      justify-content: space-between;
      position: sticky;
      top: 0;
      z-index: 2;
      backdrop-filter: blur(4px);
    }

    .logo {
      display: flex;
      align-items: center;
      background-color: #b7b7b7;
      border-radius: 50px;
      padding: 2px;
      margin-left: 8px;
    }

    .button-9 {
      appearance: button;
      backface-visibility: hidden;
      background-color: #ffffff00;
      border-radius: 3px;
      border-width: 0;
      box-sizing: border-box;
      cursor: pointer;
      font-family: -apple-system, system-ui, "Segoe UI", Roboto,
        "Helvetica Neue", Ubuntu, sans-serif;
      font-size: 100%;
      outline: none;
      transition: all 0.2s, box-shadow 0.08s ease-in;
      width: 100%;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    .button-9:disabled {
      cursor: default;
    }

    .button-9:active {
      box-shadow: rgba(134, 134, 134, 0.753) 0 0 7px 3px;
    }

    .button-9:hover {
      background-color: #ececec78;
    }

    .dashboard {
      max-width: 1400px;
      margin: 0 auto;
      padding: 1rem;
    }

    h1 {
      text-align: center;
      /* margin-bottom: 1rem; */
      font-size: 1.8rem;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--accent-color);
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 0.75rem;
    }

    .card {
      background-color: var(--card-bg);
      border-radius: 8px;
      padding: 0.75rem;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      transition: transform 0.3s ease, box-shadow 0.3s ease,
        background-color 0.3s;
    }

    .card:hover {
      transform: translateY(-3px);
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.15);
    }

    .card h2 {
      display: flex;
      align-items: center;
      gap: 0.25rem;
      margin-bottom: 0.5rem;
      font-size: 1rem;
      color: var(--accent-color);
    }

    .icon {
      font-size: 1.2rem;
    }

    .badge {
      display: inline-block;
      padding: 0.15rem 0.3rem;
      background-color: var(--success-color);
      color: var(--bg-color);
      border-radius: 12px;
      font-size: 0.7rem;
      font-weight: bold;
    }

    .progress-bar {
      width: 100%;
      height: 10px;
      background-color: rgba(128, 128, 128, 0.2);
      border-radius: 50px;
      overflow: hidden;
      margin: 0.25rem 0;
    }

    .progress-fill {
      height: 100%;
      background-color: var(--accent-color);
      transition: width 0.5s ease-out;
    }

    .memory-usage {
      margin-top: 0.5rem;
    }

    .memory-item {
      margin-bottom: 0.5rem;
    }

    .terminal {
      background-color: #000;
      color: #00ff00;
      padding: 0.5rem;
      border-radius: 4px;
      height: 150px;
      overflow-y: auto;
      font-family: "Courier New", Courier, monospace;
      font-size: 0.8rem;
    }

    .log-entry {
      margin-bottom: 0.25rem;
      opacity: 0;
      animation: fadeIn 0.5s ease-out forwards;
    }

    @keyframes fadeIn {
      to {
        opacity: 1;
      }
    }

    .status-indicator {
      display: inline-block;
      width: 8px;
      height: 8px;
      border-radius: 50%;
      margin-right: 0.25rem;
    }

    .status-ok {
      background-color: var(--success-color);
    }

    .status-warning {
      background-color: var(--warning-color);
    }

    .status-error {
      background-color: var(--danger-color);
    }

    .chart-container {
      position: relative;
      height: auto;
      margin-top: 0.5rem;
    }

    .theme-toggle {
      color: var(--bg-color);
      border: none;
      border-radius: 50%;
      width: 30px;
      height: 30px;
      font-size: 1.2rem;
      cursor: pointer;
      transition: background-color 0.3s, transform 0.3s;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    .theme-toggle:hover {
      transform: scale(1.1);
    }

    @media (max-width: 768px) {
      .dashboard {
        padding: 0.5rem;
      }

      h1 {
        font-size: 1.5rem;
      }

      .card {
        padding: 0.5rem;
      }
    }
  </style>
</head>

<body>
  <div class="nav">
    <div class="logo">
      <svg viewBox="0 0 45 45" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path
          d="M23.0261 7.548V11.578L27.0521 9.253L28.0521 10.986L23.0261 13.887V20.815L29.0261 17.351V11.548H31.0261V16.196L34.5171 14.182L35.5171 15.914L32.0261 17.929L36.0521 20.253L35.0521 21.986L30.0261 19.083L24.0261 22.547L30.0271 26.012L35.0521 23.11L36.0521 24.842L32.0261 27.166L35.5171 29.182L34.5171 30.914L31.0261 28.899V33.548H29.0261V27.744L23.0261 24.279V31.208L28.0521 34.11L27.0521 35.842L23.0261 33.517V37.548H21.0261V33.517L17.0001 35.842L16.0001 34.11L21.0261 31.208V24.279L15.0261 27.743V33.548H13.0261V28.898L9.53606 30.914L8.53606 29.182L12.0251 27.166L8.00006 24.842L9.00006 23.11L14.0251 26.011L20.0251 22.547L14.0261 19.083L9.00006 21.986L8.00006 20.253L12.0261 17.929L8.53606 15.914L9.53606 14.182L13.0261 16.196V11.548H15.0261V17.351L21.0261 20.815V13.887L16.0001 10.986L17.0001 9.253L21.0261 11.578V7.548H23.0261Z"
          fill="#3C3C3C" />
      </svg>
    </div>
    <h1>Air Wall</h1>
    <ul>
      <li>
        <button class="button-9" role="button">
          <svg id="terminal" viewBox="0 0 24 24">
            <g stroke="none" stroke-width="1" fill="none" fill-rule="evenodd">
              <g fill="#000000" fill-rule="nonzero">
                <path
                  d="M4,4 L20,4 C21.1045695,4 22,4.8954305 22,6 L22,18 C22,19.1045695 21.1045695,20 20,20 L4,20 C2.8954305,20 2,19.1045695 2,18 L2,6 C2,4.8954305 2.8954305,4 4,4 Z M4,6 L4,18 L20,18 L20,6 L4,6 Z M12,14 L18,14 L18,16 L12,16 L12,14 Z M10.9852814,11.5710678 L7.44974747,15.1066017 L6.03553391,13.6923882 L8.15685425,11.5710678 L6.03553391,9.44974747 L7.44974747,8.03553391 L10.9852814,11.5710678 Z"
                  id="Shape"></path>
              </g>
            </g>
          </svg>
        </button>
      </li>
      <li>
        <button class="button-9" role="button">
          <svg viewBox="0 0 24 24" fill="none">
            <path fill-rule="evenodd" clip-rule="evenodd"
              d="M6.31317 12.463C6.20006 9.29213 8.60976 6.6252 11.701 6.5C14.7923 6.6252 17.202 9.29213 17.0889 12.463C17.0889 13.78 18.4841 15.063 18.525 16.383C18.525 16.4017 18.525 16.4203 18.525 16.439C18.5552 17.2847 17.9124 17.9959 17.0879 18.029H13.9757C13.9786 18.677 13.7404 19.3018 13.3098 19.776C12.8957 20.2372 12.3123 20.4996 11.701 20.4996C11.0897 20.4996 10.5064 20.2372 10.0923 19.776C9.66161 19.3018 9.42346 18.677 9.42635 18.029H6.31317C5.48869 17.9959 4.84583 17.2847 4.87602 16.439C4.87602 16.4203 4.87602 16.4017 4.87602 16.383C4.91795 15.067 6.31317 13.781 6.31317 12.463Z"
              stroke="#000000" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" />
            <path
              d="M9.42633 17.279C9.01212 17.279 8.67633 17.6148 8.67633 18.029C8.67633 18.4432 9.01212 18.779 9.42633 18.779V17.279ZM13.9757 18.779C14.3899 18.779 14.7257 18.4432 14.7257 18.029C14.7257 17.6148 14.3899 17.279 13.9757 17.279V18.779ZM12.676 5.25C13.0902 5.25 13.426 4.91421 13.426 4.5C13.426 4.08579 13.0902 3.75 12.676 3.75V5.25ZM10.726 3.75C10.3118 3.75 9.97601 4.08579 9.97601 4.5C9.97601 4.91421 10.3118 5.25 10.726 5.25V3.75ZM9.42633 18.779H13.9757V17.279H9.42633V18.779ZM12.676 3.75H10.726V5.25H12.676V3.75Z"
              fill="#000000" />
          </svg>
        </button>
      </li>
      <li>
        <button class="theme-toggle" id="themeToggle">🌓</button>
      </li>
    </ul>
  </div>
  <div class="dashboard">
    <div class="grid">
      <div class="card">
        <h2><span class="icon">ℹ️</span>Network Info</h2>
        <div id="netInfo"></div>
        <br>
        <br>
        <h2><span class="icon">⏱️</span> Uptime</h2>
        <p style="font-size: 1.2em; font-weight: bold" id="uptime"></p>
      </div>
      <div class="card">
        <h2><span class="icon">🖥️</span> System Info</h2>
        <div id="chipInfo"></div>
      </div>
      <div class="card">
        <h2><span class="icon">🛜</span> Signal Strength</h2>
        <div class="progress-bar">
          <div class="progress-fill" id="signalStrength"></div>
        </div>
        <p id="signalInfo"></p>
        <br>
        <h2><span class="icon">💾</span> Memory Usage</h2>
        <div class="memory-usage">
          <div class="memory-item">
            <p>Heap: 75% (768/1024 MB)</p>
            <div class="progress-bar">
              <div class="progress-fill" style="width: 75%"></div>
            </div>
          </div>
          <div class="memory-item">
            <p>Free: 25% (256/1024 MB)</p>
            <div class="progress-bar">
              <div class="progress-fill" style="width: 25%"></div>
            </div>
          </div>
        </div>
      </div>
      <div class="card">
        <h2><span class="icon">⚠️</span> Threat Detection</h2>
        <span class="badge">Active</span>
        <p>
          <span class="status-indicator status-ok"></span>No threats detected in 24h
        </p>
        <div class="chart-container">
          <canvas id="threatChart" style="height: 100%; width: auto; margin: auto;"></canvas>
        </div>
      </div>
      <div class="card">
        <h2><span class="icon">🛡️</span> Firewall Status</h2>
        <span class="badge">Active</span>
        <p>
          <span class="status-indicator status-ok"></span>All systems operational
        </p>
        <!-- <div class="chart-container"> -->
          <svg viewBox="0 0 24 24" fill="none" style="width: 50%;margin-left: 25%;">
            <path d="M9 12L11 14L15 9.99999M20 12C20 16.4611 14.54 19.6937 12.6414 20.683C12.4361 20.79 12.3334 20.8435 12.191 20.8712C12.08 20.8928 11.92 20.8928 11.809 20.8712C11.6666 20.8435 11.5639 20.79 11.3586 20.683C9.45996 19.6937 4 16.4611 4 12V8.21759C4 7.41808 4 7.01833 4.13076 6.6747C4.24627 6.37113 4.43398 6.10027 4.67766 5.88552C4.9535 5.64243 5.3278 5.50207 6.0764 5.22134L11.4382 3.21067C11.6461 3.13271 11.75 3.09373 11.857 3.07827C11.9518 3.06457 12.0482 3.06457 12.143 3.07827C12.25 3.09373 12.3539 3.13271 12.5618 3.21067L17.9236 5.22134C18.6722 5.50207 19.0465 5.64243 19.3223 5.88552C19.566 6.10027 19.7537 6.37113 19.8692 6.6747C20 7.01833 20 7.41808 20 8.21759V12Z" stroke="#00DD33" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
        <!-- </div> -->
      </div>
      <div class="card">
        <h2><span class="icon">🌐</span> Network Traffic</h2>
        <div class="chart-container">
          <canvas id="trafficChart"></canvas>
        </div>
      </div>
      <div class="card" style="grid-column: 1 / -1">
        <h2><span class="icon">💻</span> System Logs</h2>
        <div class="terminal" id="serial_output">
        </div>
      </div>
    </div>
  </div>
  <script>
    const xValues = ["Allowed", "Not Allowed"];
    const yValues = [80, 20];
    const barColors = [
      "#b91d47",
      "#00aba9"
    ];


    new Chart("threatChart", {
      type: "doughnut",
      data: {
        labels: xValues,
        datasets: [{
          backgroundColor: barColors,
          data: yValues
        }]
      },
      options: {
        title: {
          display: false,
          text: "World Wide Wine Production 2018"
        }
      }
    });

    const trafficxValues = [1,2,3,4,5,6,7,8,9,10];
    const trafficyValues = [7,8,8,56,45,32,10,11,14,14];
    new Chart("trafficChart", {
      type: "line",
      data: {
        labels: trafficxValues,
        datasets: [{
          fill: false,
          lineTension: 0,
          backgroundColor: "rgba(0,0,255,1.0)",
          borderColor: "rgba(0,0,255,0.5)",
          data: trafficyValues
        }]
      },
      options: {
        legend: { display: false },
        scales: {
          yAxes: [{ ticks: { min: 0, max: 100 } }],
        }
      }
    });


    const themeToggle = document.getElementById("themeToggle");
    const htmlElement = document.documentElement;

    themeToggle.addEventListener("click", () => {
      const isLightTheme = htmlElement.dataset.theme === "light";
      htmlElement.dataset.theme = isLightTheme ? "dark" : "light";
      themeToggle.textContent = isLightTheme ? "🌞" : "🌓";
    });

  </script>
  <script>
    let serial_output = document.getElementById("serial_output");
    let memoryInfo = document.getElementById("memoryInfo");
    let flashMemoryInfo = document.getElementById("flashMemoryInfo");
    let messageLimit = 50;  // Set the number of messages you want to keep
    let messages = [];      // Array to store incoming messages

    let netInfo = document.getElementById("netInfo");
    let chipInfo = document.getElementById("chipInfo");
    let signalStrength = document.getElementById("signalStrength");
    let signalInfo = document.getElementById("signalInfo");
    let terminal = document.querySelector(".terminal");

    let ws = new WebSocket("ws://" + window.location.hostname + ":81");
    ws.onmessage = function (event) {
      try {
        let data = JSON.parse(event.data);
        console.log(data);
        messages.push(data.serialData);
        if (messages.length > messageLimit) {
          messages.shift();  // Remove the first (oldest) message
        }
        serial_output.innerHTML = "";
        messages.forEach(msg => {
          serial_output.innerHTML += msg + "<br>";
        });

        netInfo.innerHTML = data.netInfo;
        chipInfo.innerHTML = data.chipInfo;

        let power = 100 - (-(data.signalStrength));
        if (power >= 60) {
          document.getElementById("signalStrength").style.backgroundColor = "#00dc09";
        }
        else if (power <= 60 && power >= 40) {
          document.getElementById("signalStrength").style.backgroundColor = "#5852ff";
        } else {
          document.getElementById("signalStrength").style.backgroundColor = "#fe0000";
        }
        signalStrength.style.width = `${power}%`;
        signalInfo.innerHTML = `${power}% (${data.signalStrength}) dBm`;

        const seconds = data.runtimeInfo;
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const remainingSeconds = seconds % 60;
        uptime.innerHTML = `${hours}h ${minutes}m ${remainingSeconds}s`;

      } catch (error) {
        console.error("Error parsing WebSocket message: ", error);
      }
      terminal.scrollTop = terminal.scrollHeight;
    };
  </script>
</body>

</html>
)====";

unsigned long previousMillis = 0;  // Stores the last time the LED pattern was updated
int ledState = LOW;                // Current state of the LED
int patternStep = 0;               // Step in the LED pattern
const long interval = 100;         // Interval between pattern steps (100 milliseconds)

// Non-blocking LED Pattern function
void ledPattern(int pid) {
  unsigned long currentMillis = millis();  // Get the current time

  if (currentMillis - previousMillis >= interval) {
    // Save the last time the LED was updated
    previousMillis = currentMillis;

    // Pattern 1: Blink
    if (pid == 1) {
      ledState = (ledState == LOW) ? HIGH : LOW;  // Toggle LED state
      digitalWrite(2, ledState);                  // Update LED state
    }
    // Pattern 2: Breathing effect
    else if (pid == 2) {
      if (patternStep <= 255) {
        analogWrite(2, patternStep);  // Fade in (0 to 255)
        patternStep++;
      } else if (patternStep <= 510) {
        analogWrite(2, 510 - patternStep);  // Fade out (255 to 0)
        patternStep++;
      }
      if (patternStep > 510) {
        patternStep = 0;  // Reset pattern step
      }
    }
    // Pattern 3: Chasing effect (3 steps)
    else if (pid == 3) {
      if (patternStep == 0) {
        digitalWrite(2, HIGH);  // LED ON
      } else if (patternStep == 1) {
        digitalWrite(2, LOW);   // LED OFF
      } else if (patternStep == 2) {
        digitalWrite(2, HIGH);  // LED ON
      }
      patternStep = (patternStep + 1) % 3;  // Loop over 3 steps
    }
  }
}

void addLog(String newLog) {
  // Store log in circular buffer
  logEntries[logIndex] = newLog;
  logIndex = (logIndex + 1) % 20; // Wrap index when full
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client connected to WebSocket: %u\n", num);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Client disconnected: %u\n", num);
  }
}

// Function to format time in hh:mm:ss format from millis()
String getFormattedTime() {
  unsigned long currentMillis = millis();
  unsigned long totalSeconds = currentMillis / 1000;
  unsigned long seconds = totalSeconds % 60;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long hours = (totalSeconds / 3600) % 24;

  char timeString[9];
  sprintf(timeString, "%02lu:%02lu:%02lu", hours, minutes, seconds);  // Format time as hh:mm:ss

  return String(timeString);
}


// Buffer to store Serial data temporarily
String serialBuffer = ""; // check

String chipInfo = ""; // check

int freeHeap = 0;
int minFreeHeap = 0;
int totalHeap = 0;

String netInfo = ""; //check

int runtimeInfo = 0; //check

int signalStrength = 0;//check

void getSystemInfo() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  chipInfo = "<p>Chip Model: <strong>" + String(ESP.getChipModel()) + "</strong></p>";
  chipInfo += "<p>Chip Revision: <strong>" + String(ESP.getChipRevision()) + "</strong></p>";
  chipInfo += "<p>Number of Cores: <strong>" + String(chip_info.cores) + "</strong></p>";
  chipInfo += "<p>CPU Frequency: <strong>" + String(ESP.getCpuFreqMHz()) + " MHz</strong></p>";
  chipInfo += "<p>Flash Size: <strong>" + String(spi_flash_get_chip_size() / (1024 * 1024)) + " MB</strong></p>";

  freeHeap = esp_get_free_heap_size();
  minFreeHeap = esp_get_minimum_free_heap_size();
  totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);

  netInfo = "<p>IP Address: <strong>" + WiFi.localIP().toString() + "</strong></p>";
  netInfo += "<p>MAC Address: <strong>" + WiFi.macAddress() + "</strong></p>";
  netInfo += "<p>SSID: <strong>" + WiFi.SSID() + "</strong></p>";
  
  signalStrength = WiFi.RSSI();
  
  runtimeInfo =millis() / 1000;
}

void sendSerialToWebClients() {
  getSystemInfo();
  if (serialBuffer.length() > 0) {
    JsonDocument jsonDoc;  // Create a JSON object to hold both serial data and system info
    jsonDoc["serialData"] = serialBuffer;
    jsonDoc["chipInfo"] = chipInfo;
    jsonDoc["freeHeap"] = freeHeap;
    jsonDoc["minFreeHeap"] = minFreeHeap;
    jsonDoc["totalHeap"] = totalHeap;
    jsonDoc["signalStrength"] = signalStrength;
    jsonDoc["netInfo"] = netInfo;
    jsonDoc["runtimeInfo"] = runtimeInfo;
    
    String jsonString;
    serializeJson(jsonDoc, jsonString);  // Serialize the JSON object to a string

    webSocket.broadcastTXT(jsonString);  // Send the JSON string to WebSocket clients

    // Clear buffer after sending
    serialBuffer = "";
    chipInfo = "";
    netInfo = "";
    runtimeInfo = 0;
    signalStrength = 0;
  }
}



// Function to provide the HTML page with current LED state
String getHtmlPage() {
  String page = htmlPage;
  return page;
}

void connectWifi() {
  // Check if already connected
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  // Start the WiFi connection process
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  // Wait until connected or timeout occurs
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(500);
    Serial.print(".");
    retryCount++;
    ledPattern(3);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to WiFi.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi.");
  }
}

String generateRandomPassword(int length) {
  String password = "";
  for (int i = 0; i < length; i++) {
    int randomIndex = random(0, charset.length());
    password += charset[randomIndex];
  }
  Serial.println(password);
  return password;
}

// If hacking is detected, ESP32 tries to connect to WiFi with a fake password
void prevent() {
  int passwordTest = 0; 
  
  while (WiFi.status() != WL_CONNECTED && passwordTest <= 10) {
    WiFi.disconnect();
    WiFi.begin(ssid, generateRandomPassword(passwordLength));
    delay(500);
    Serial.println("Connecting to WiFi with fake password...");
    addLog("Attack preventing");
    passwordTest++;
  }
  delay(1000);
  deauthTrying = 0;
  connectWifi();
}

// Function to detect deauth packets
void sniffer_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{
  
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  if (pkt->payload[0] == 0xC0){
    Serial.println("Deauth packet detected!"); // Check for deauthentication packet
    addLog("Deauth packet detected!");
    deauthTrying++;
    Serial.println(deauthTrying);
  }
  else if(deauthTrying >= 20){
    prevent();
  }
//  else {
//    WiFi.disconnect();
//  }
}

// Function to scan for nearby WiFi networks, check for duplicate SSIDs and if one is open
void checkDuplicateSSIDs() {
  WiFi.disconnect();
  int n = WiFi.scanNetworks();  // Scan for available networks
  Serial.println("Scan complete.");

  if (n == 0) {
    Serial.println("No networks found");
    return;
  }

  Serial.print(n);
  Serial.println(" networks found.");

  // Loop through available networks and check for duplicates and open networks
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (WiFi.SSID(i) == WiFi.SSID(j)) {  // Check if SSIDs are duplicate
        Serial.print("Duplicate SSID detected: ");
        Serial.println(WiFi.SSID(i));

        // Check if any of the networks with this SSID is open (no security)
        if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN || WiFi.encryptionType(j) == WIFI_AUTH_OPEN) {
          Serial.print("Open network detected for SSID: ");
          Serial.println(WiFi.SSID(i));

          // Trigger defense mechanism, e.g., prevent connection to this open network
          prevent();
        }
      }
    }
  }

  // Clear the list of networks to avoid overflow of memory
  WiFi.scanDelete();
}


//fake terminal
// Define arrays for different types of log messages
String systemActions[] = {
  "[INFO] Installing package: ", 
  "[INFO] Downloading update: ", 
  "[INFO] Configuring: ", 
  "[INFO] Starting service: ", 
  "[INFO] Stopping service: ",
  "[INFO] Setting up: ", 
  "[INFO] Cleaning up: ",
  "[ALERT] Unauthorized access attempt detected at: ", 
  "[WARNING] High CPU usage on core: ", 
  "[INFO] Connection established to remote server: ", 
  "[ALERT] Firewall blocked external access from IP: ", 
  "[INFO] Update completed for package: "
};

// Define arrays for package names and IPs
String packageNames[] = {
  "web-server",
  "openssh-server",  
  "firewall", 
  "vpn-service"
};

String ipAddresses[] = {
  "192.168.1.5", 
  "192.168.1.10", 
  "10.0.0.2", 
  "172.16.0.7", 
  "203.0.113.5", 
  "198.51.100.12", 
  "8.8.8.8", 
  "192.168.0.254"
};

// Random Core Numbers
int cores[] = {0, 1};

// Number of system actions, packages, and IPs
const int numActions = sizeof(systemActions) / sizeof(systemActions[0]);
const int numPackages = sizeof(packageNames) / sizeof(packageNames[0]);
const int numIPs = sizeof(ipAddresses) / sizeof(ipAddresses[0]);
const int numCores = sizeof(cores) / sizeof(cores[0]);

String faketerminal(){
  int randomActionIndex = random(0, numActions);
  int randomPackageIndex = random(0, numPackages);
  int randomIPIndex = random(0, numIPs);
  int randomCoreIndex = random(0, numCores);

  String terminaltext = "";
  // Simulate different types of log output based on the random action
  if (randomActionIndex < 9) { 
    // Actions related to packages
    terminaltext = systemActions[randomActionIndex];
    terminaltext += packageNames[randomPackageIndex];
  } else if (randomActionIndex == 9) {
    // Unauthorized access attempts with an IP
    terminaltext = systemActions[randomActionIndex];
    terminaltext += ipAddresses[randomIPIndex];
  } else if (randomActionIndex == 10) {
    // High CPU usage alerts on a random core
    terminaltext = systemActions[randomActionIndex];
    terminaltext += cores[randomCoreIndex];
  } else if (randomActionIndex == 11 || randomActionIndex == 12) {
    // Connection established or firewall blocked external access from an IP
    terminaltext = systemActions[randomActionIndex];
    terminaltext += ipAddresses[randomIPIndex];
  } else {
    // Malware scan or update completed
    terminaltext = systemActions[randomActionIndex];
    terminaltext += packageNames[randomPackageIndex];
  }
  return terminaltext;
}


void showSplashScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.println("AIR WALL");
  display.display();
  delay(2000); // Show splash screen for 2 seconds
  display.clearDisplay();
}

void clearScreen(int setClean){
  if(cleanCount == 0 && setClean == 1){
    display.fillRect(0, 16, SCREEN_WIDTH, SCREEN_HEIGHT - 21, SSD1306_BLACK);
    cleanCount++;
  }else if(cleanCount != 0 || setClean != 1){   
    display.fillRect(0, 16, SCREEN_WIDTH, SCREEN_HEIGHT - 21, SSD1306_BLACK);
  }
}

void displayYellowText() {

  // Display "AIR WALL" in the yellow portion (top)
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_BLACK); // Clear yellow section
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0);
  display.println("AIR WALL");
  display.display();
}
void displayScreenIndicator() {
  cleanCount = 0;
  // Clear the bottom section for the indicator
  display.fillRect(0, SCREEN_HEIGHT - 4, SCREEN_WIDTH, 4, SSD1306_BLACK);

  // Draw three circles at the bottom center for screen indicators
  int baseX = 53; // Center of the screen horizontally
  int y = SCREEN_HEIGHT - 3; // Vertical position for the indicators
  int spacing = 10; // Space between circles

  for (int i = 0; i < 3; i++) {
    if (i == currentScreen) {
      // Active screen: draw a filled circle
      display.fillCircle(baseX + (i * spacing), y, 2, SSD1306_WHITE);
    } else {
      // Inactive screens: draw an unfilled circle
      display.drawCircle(baseX + (i * spacing), y, 2, SSD1306_WHITE);
    }
  }

  display.display();
}

void displayWiFiScreen() {
  displayYellowText(); // Always show "AIR WALL" in the yellow portion
  clearScreen(1);
  // Show WiFi status in the blue portion
  display.fillRect(0, 16, SCREEN_WIDTH, SCREEN_HEIGHT - 26, SSD1306_BLACK); // Clear blue section
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 18);

  if (WiFi.status() == WL_CONNECTED) {
    // Show IP address
    // display.println("WiFi Connected");
    display.print("IP:");
    display.println(WiFi.localIP());
    display.print("MAC:");
    display.println(WiFi.macAddress());
    display.print("SSID:");
    display.print(WiFi.SSID());
  } else {
    // Show connecting animation
    static int dotCount = 0;
    display.println("Connecting to WiFi");
    for (int i = 0; i < dotCount; i++) {
      display.print(".");
    }
    dotCount = (dotCount + 1) % 4; // Cycle between 0-3 dots
  }

  display.display();

  // Show screen indicator
  displayScreenIndicator();
}

void displayScreen1() {
    static unsigned long lastLogUpdate = 0;

    if (millis() - lastLogUpdate >= 1000) { // Update logs every 1000ms
        lastLogUpdate = millis();

        displayYellowText(); // Always show "AIR WALL" in the yellow portion
        clearScreen(2);      // Clear specific portions of the screen

        // Display the heading
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(1, 16);
        display.println("LOG:");

        // Scroll the logs in the visible log area
        int yPosition = LOG_BOX_Y + 2; // Start drawing a few pixels below the top of the box
        for (int i = 0; i < 4; i++) {  // Display a maximum of 5 lines in the log box
            int index = (logIndex + i) % 20;
            display.setCursor(LOG_BOX_X + 2, yPosition + i * LINE_SPACING);
            display.print(logEntries[index]);
        }

        display.display(); // Commit changes to the display
        displayScreenIndicator(); // Show screen indicator
    }
}

void displayScreen2() {
  displayYellowText(); // Always show "AIR WALL" in the yellow portion
  clearScreen(1);
  // Show content for Screen 2
  display.fillRect(0, 16, SCREEN_WIDTH, SCREEN_HEIGHT - 26, SSD1306_BLACK); // Clear blue section
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 18);
  display.println("Screen 2");
  display.println("This is the second screen.");
  display.display();

  // Show screen indicator
  displayScreenIndicator();
}

// Setup function
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  pinMode(2, OUTPUT);
    // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
      // Show splash screen
  showSplashScreen();

  // Initialize the switches as input with pull-up resistors
  pinMode(switch1Pin, INPUT_PULLUP);
  pinMode(switch2Pin, INPUT_PULLUP);
  
  connectWifi();
  WiFi.mode(WIFI_STA);  // Set ESP32 in Station Mode
  esp_wifi_set_promiscuous(true);  // Enable promiscuous mode for packet sniffing
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);  // Set callback for sniffer

  // Define routes for the web server
  server.on("/", []() {
    server.send(200, "text/html", getHtmlPage());
  });

  // Start the web server
  server.begin();
  Serial.println("Web server started!");
  addLog("Web server started!");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Web server and WebSocket server started.");
  randomSeed(analogRead(0));


}


void loop() {
  // Read the state of the switches
  bool switch1State = digitalRead(switch1Pin);
  bool switch2State = digitalRead(switch2Pin);

  // Handle switch 1 press
  if (switch1State == LOW && (millis() - lastDebounceTime1 > debounceDelay)) {
    lastDebounceTime1 = millis(); // Update debounce time
    currentScreen++;
    Serial.println("Switch 1 pressed.");
    addLog("Switch 1 pressed.");
  }

  // Handle switch 2 press
  if (switch2State == LOW && (millis() - lastDebounceTime2 > debounceDelay)) {
    lastDebounceTime2 = millis(); // Update debounce time
    currentScreen--;
    Serial.println("Switch 2 pressed.");
    addLog("Switch 2 pressed.");
  }
  
  // Check Serial input for screen changes
  if (Serial.available()) {
    char input = Serial.read();
    if (input == '1') currentScreen = 1; // Switch to Screen 1
    if (input == '2') currentScreen = 2; // Switch to Screen 2
    if (input == '0') currentScreen = 0; // Switch to WiFi screen
  }
  // Serial.println(cleanCount);
  // Display based on the current screen
  switch (currentScreen) {
    case 0:
      displayWiFiScreen();
      break;
    case 1:
      displayScreen1();
      break;
    case 2:
      displayScreen2();
      break;
  }
  // addLog("Sensor value: " + String(random(0, 100))); //for test the log data
  
  //checking for reconnect to wifi 
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    connectWifi();
  }
  ledPattern(2);
  // Handle incoming client requests
  server.handleClient();
  webSocket.loop(); // Handle WebSocket events

  
  // Infinite loop: print "Hello World" with timestamp to Serial and buffer it for the web page
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 1000) {  // Print every 1 second
    String message = getFormattedTime() + faketerminal();  // Add timestamp to message
//    Serial.println(message);
    serialBuffer += message + "\n";  // Append to buffer
    lastTime = millis();
  }
  // Send the buffered Serial data to the WebSocket clients
  sendSerialToWebClients();
}
