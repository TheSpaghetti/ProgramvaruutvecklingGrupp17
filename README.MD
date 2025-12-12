# PA1484 – Software Development: Project (Group 17)

## Introduction
This project contains a weather dashboard built using PlatformIO for a LilyGO AMOLED device, powered by an ESP32 microcontroller. The system retrieves weather data from the SMHI API, presenting it on the devices touchscreen display.
The goal of this project is to deliver an intuitive way to check current weather conditions, forecasts and past weather pattern on a LilyGO display.

The projects main functionalities include:
- Data fetching via Wi-Fi from SMHI
- Interactive touchscreen navigation
- A seven day weather forecast
- A graph showing the past 90 days of weather data
- Configurable data types (temperature, humidity, etc) and weather data locations (Karlskrona, Stockholm, etc)
- Configurable default settings and persistant device memory

## Getting started
This project will work regardless of the operating system you use, with separate guides for each below. Each of these tutorials is done on a fresh install of Visual Studio Code, which is the IDE you must use to complete this project.

### Prerequisites
1. Install Git for your device.
2. Install and run [Visual Studio Code](https://code.visualstudio.com/download).
3. In VS Code, install the ["PlatformIO IDE"](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension.
4. **(Windows/Linux only)** Install the Clang compiler and C++ extension for Visual Studio Code.
5. **(Linux only)** Run the following command to add access to the `ttyACM0` port in order to avoid complications:
   * `sudo adduser $USER $(stat --format="%G" /dev/ttyACM0 )`

### How to upload this program your device
1. Clone this repository.
2. Open the locally cloned repository with Visual Studio Code
    * If the "Do you trust the authors of the files in this folder?" dialog appears, click on "Yes, I trust the authors"
3. Open up the file `project/project.ino`
4. Change `WIFI_SSID` and `WIFI_PASSWORD` values to your Wi-Fi name and password.
4. Connect your ESP32 to your computer via a USB cable.
5. Build and upload the project to the device. See screenshot and the instructions below to put your device in download mode:
   1. Press and hold the **BOOT** button , While still pressing the **BOOT** button
   2. Release the **RST** button
   3. Release the **BOOT** button
   4. The USB port should be fixed and will not flash again. You can click Upload.
   5. Press the **RST** button to exit download mode

![[screenshot](./assets/screenshot.png)](./assets/screenshot.png)

---

### Using the device
After the program has been uploaded the device can be disconnected from your device and can run as long as it is connected to power.
If the device needs to run in a different location or your Wi-Fi credentials have changed then the program needs to be updated accordingly and uploaded again.

The program is operated via touchscreen interaction and has four different views, these can be viewed via swiping left or right on the device.
1. The first screen contains the version number and developer information.
2. Swiping right on the first screen shows a second screen that contains a seven day weather forcast.
3. Swiping right again shows the third screen that contains a historical graph for weather data.
    * The graph can be navigated via touchscreen input on the slider at the bottom.
4. Swiping right yet again shows the final fourth screen, where settings for weather data can be configured.
    * Pressing on the data type or city opens a dropdown menu, where they can be changed to other options.
    * Pressing the **Save default** button will save the currently selected data type and city as the default on the device.
    * Pressing the **Use default** button will load the default data type and city.

## Features
A list of complete and excluded features:
#### Start-up
- [x] **US1.1C:** As a user, I want to see a starting screen to display the current program
version and group number on the first screen.
- [x] **US1.2C:** As a user, I want to see the weather forecast for the next 7 days for the
selected city on the second screen in terms of temperature and weather conditions
with symbols (e.g., clear sky, rain, snow, thunder) per day at 12:00.
- [x] **US1.3:** As a user, I want to have a screen to view weather forecast data.
#### Tile Navigation & Access
- [x] **US2.1:** As a user, I want to be able to navigate between different screens (like forecast
screen) by sliding a finger over the touch screen.
#### Weather Data Display
- [x] **US3.1:** As a user, I want to have a screen to view historical weather data.
- [x] **US3.2D:** As a user, on the third screen I want to view the latest months (SMHI API
period: latest-months) of historical hourly data for selected weather parameter in the
selected city, using a slider to interact with the historical graph by scrolling where a
depleted slider corresponds to the oldest datapoint and a full slider corresponds to the
latest datapoint.
#### Settings & Configuration
- [x] **US4.1:** As a user, on the fourth screen, I want to access a single settings screen to
configure both the city and weather parameter options.
- [x] **US4.2B:** As a user, I want to select from four weather parameters, namely
temperature (1), humidity (6), wind speed (4), and Air pressure (9), using a dropdown
list, to customize the historical graph.
- [x] **US4.3B:** As a user, I want to select from five different cities, namely
Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), and
Kiruna(180940), using a dropdown, to view their weather data for the historical data
and starting screen forecast.
- [x] **US4.4:** As a user, I want to reset the selected city and weather parameter to default
using a button.
- [x] **US4.5:** As a user, I want to set my default city and weather parameter to the current
selection using a button, so they are automatically selected when I start the device.
- [x] **US4.6:** As a user, I want the device to store my default city and weather parameter so
that they are retained even after a restart.
#### Extended Forecast & Visualization
- [ ] **US5.1:** As a user, I want to access a screen to view the weather forecast for all of
Sweden.
- [ ] **US5.2:** As a user, I want to see the temperature forecast for each administrative area
of Sweden (“Landskap”) on a map, with:
* A Color-coded temperature zone per each administrative area, using the
center of each area to access the forecast.
* A looped animation displaying the hourly forecast for the next 24 hours.
