# PA1484 – Software Development: Project (Group 17)

## Template information
This template should help your team write a good README file for your project.
(Your file should be named `README.md` in your project directory.) You are of
course free to add more sections to your README if you want to.
Readme-files on GitHub are formatted using Markdown. You can find information about
how to format using Markdown here:
https://docs.github.com/en/get-started/writing-on-github/getting-started-with-
writing-and-formatting-on-github/basic-writing-and-formatting-syntax
Your README file should include the at least the following sections:

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

### General
1. Install Visual Studio Code
   * Visit [Visual Studio Code's website](https://code.visualstudio.com/download) and download the latest version or use the package manager of your system.
   * Run `Visual Studio Code` and follow the steps.
2. Install the correct extension.
   * Head over to the extensions tab on your left.
   * Search for ["PlatformIO IDE"](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) and install it.

### Windows
3. Install the Clang compiler and C++ extension for Visual Studio Code.
   * (optional) If you want/need to use Windows Subsystem for Linux (WSL) have at the [documentation](https://code.visualstudio.com/docs/remote/wsl).

### Linux
3. Install the Clang compiler and C++ extension for Visual Studio Code.
4. Run the following command to add access to the `ttyACM0` port in order to avoid complications [1]
   * `sudo adduser $USER $(stat --format="%G" /dev/ttyACM0 )`

### MacOS
3. Run `git -v` in the Terminal app to check whether git and the macOS command line tools are installed. If the command line tools are not installed, a pop up will appear to install it.

## How to run the program
1. Open the locally cloned repository with Visual Studio Code
    * If the "Do you trust the authors of the files in this folder?" dialog appears, click on "Yes, I trust the authors"
2. Open up the file `project/project.ino`
3. Connect your ESP32 to your computer via a USB cable.
4. Build and upload the project to the device. See screenshot.

![[screenshot](./assets/screenshot.png)](./assets/screenshot.png)

---



### Prerequisites
### Installation

## Building and running
This is where you explain how to make the project run.
Examples of things to describe:
- How to build/upload the program to the ESP32
- What your startup procedure looks like
- If the program accepts different arguments or modes
- How to operate the program (e.g., touchscreen interaction)

## Features
A list of complete and excluded features.
#### Start-up
- [x] **US1.1C**: As a user, I want to see a starting screen to display the current program
version and group number on the first screen.
- [x] **US1.3**: As a user, I want to have a screen to view weather forecast data.
- [x] **US1.2C**: As a user, I want to see the weather forecast for the next 7 days for the
selected city on the second screen in terms of temperature and weather conditions
with symbols (e.g., clear sky, rain, snow, thunder) per day at 12:00.
#### Tile Navigation & Access
- [x] **US2.1**: As a user, I want to be able to navigate between different screens (like forecast
screen) by sliding a finger over the touch screen.
#### Weather Data Display
- [x] **US3.1**: As a user, I want to have a screen to view historical weather data.
- [x] **US3.2D**: As a user, on the third screen I want to view the latest months (SMHI API
period: latest-months) of historical hourly data for selected weather parameter in the
selected city, using a slider to interact with the historical graph by scrolling where a
depleted slider corresponds to the oldest datapoint and a full slider corresponds to the
latest datapoint.
#### Settings & Configuration
- [x] **US4.1**: As a user, on the fourth screen, I want to access a single settings screen to
configure both the city and weather parameter options.
- [x] **US4.2B**: As a user, I want to select from four weather parameters, namely
temperature (1), humidity (6), wind speed (4), and Air pressure (9), using a dropdown
list, to customize the historical graph.
- [x] **US4.3B**: As a user, I want to select from five different cities, namely
Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), and
Kiruna(180940), using a dropdown, to view their weather data for the historical data
and starting screen forecast.
- [x] **US4.4**: As a user, I want to reset the selected city and weather parameter to default
using a button.
- [x] **US4.5**: As a user, I want to set my default city and weather parameter to the current
selection using a button, so they are automatically selected when I start the device.
- [x] **US4.6**: As a user, I want the device to store my default city and weather parameter so
that they are retained even after a restart.
#### Extended Forecast & Visualization
- [ ] **US5.1**: As a user, I want to access a screen to view the weather forecast for all of
Sweden.
- [ ] **US5.2**: As a user, I want to see the temperature forecast for each administrative area
of Sweden (“Landskap”) on a map, with:
o A Color-coded temperature zone per each administrative area, using the
center of each area to access the forecast.
o A looped animation displaying the hourly forecast for the next 24 hours.
