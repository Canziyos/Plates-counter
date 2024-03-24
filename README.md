# Automated Dish Counting System for School Cafeteria

## Overview
This project aims to address the challenge of accurately counting washed dishes in school cafeterias by developing an automated dish counting system. By leveraging a combination of sensor technologies and IoT capabilities, the system provides a reliable solution for tracking dish usage, enhancing operational efficiency, and reducing food waste.

## Key Features
- Integration of IR proximity sensors and Pololu Distance Sensors for tray detection and dish differentiation.
- Real-time data processing and analysis using an ESP32 Heltec LoRa microcontroller.
- Storage and management of counted dish information in MongoDB Atlas for easy access and retrieval.


## Getting Started
To get started with using the Automated Dish Counting System, follow these steps:
1. **Hardware Setup**: Set up the required hardware components, including the ESP32 Heltec LoRa board, IR proximity sensors, and Pololu Distance Sensors. Instructions can be found in the hardware section in the documentation.
2. **Software Installation**: Install the necessary software libraries and dependencies on the ESP32 Heltec LoRa board. Instructions can be found in the software section in the documentation.
3. **Data Storage Configuration**: Configure MongoDB Atlas for storing and managing counted dish information. follow the link [mongo DB instructions]([URL](https://www.google.com/search?q=Build+a+Totally+Serverless+REST+API+with+MongoDB+Atlas&rlz=1C5CHFA_enSE1080SE1080&oq=Build+a+Totally+Serverless+REST+API+with+MongoDB+Atlas&gs_lcrp=EgZjaHJvbWUyBggAEEUYOdIBBzMxN2owajeoAgiwAgE&sourceid=chrome&ie=UTF-8#fpstate=ive&vld=cid:96992ada,vid:FkD_tf8vkfg,st:0))
. this can be usefull example [Integrating The Things Network with MongoDB Atlas](https://www.joholtech.com/blog/2022/08/20/mongodbatlas-ttn.html)

4. **System Integration**: Integrate the sensor data with the data storage solutions as outlined in the system implementation documentation.
5. **Testing and Validation**: Test the system in a simulated or real-world environment to validate its accuracy and performance. Make any necessary adjustments or optimizations based on the results.

## Contributing
Contributions to the Automated Dish Counting System project are welcome! If you'd like to contribute, please follow these guidelines:
- Fork the repository and create a new branch for your contributions.
- Make your changes and submit a pull request, providing a clear description of the proposed changes and their rationale.
- Ensure that your code follows the project's coding standards and conventions.
- Collaborate with other contributors and maintainers to review and refine your contributions.

## License
This project is licensed under the [MIT License](LICENSE).
