#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/deviceparameters.h>
#include <iostream>
#include <exception>
#include <iomanip>
#include <stdio.h>

using namespace visiontransfer;

int main(int, char**) {
    try {
        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;

        DeviceEnumeration::DeviceList devices = deviceEnum.discoverDevices();
        if(devices.size() == 0) {
            std::cout << "No devices discovered!" << std::endl;
            return -1;
        }

        // Print devices
        std::cout << "Discovered devices:" << std::endl;
        for(unsigned int i = 0; i< devices.size(); i++) {
            std::cout << devices[i].toString() << std::endl;
        }
        std::cout << std::endl;

        // Create a connection to the parameter interface
        DeviceParameters parameters(devices[0]);

        // Output the current temperatures
        std::cout << "SOC temperature:          " << parameters.getParameter("RT_temp_soc").getCurrent<double>() << " °C" << std::endl;
        std::cout << "Left sensor temperature:  " << parameters.getParameter("RT_temp_sensor_left").getCurrent<double>() << " °C" << std::endl;
        std::cout << "Right sensor temperature: " << parameters.getParameter("RT_temp_sensor_right").getCurrent<double>() << " °C" << std::endl;

        return 0;
    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}

