#include <sys/types.h>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <fstream>

#include <visiontransfer/internalinformation.h>
#include <visiontransfer/networking.h>
#include <visiontransfer/datachannelservicebase.h>
#include <visiontransfer/datachannel-control.h>
#include <visiontransfer/datachannelservice.h>

#include <visiontransfer/datachannel-imu-bno080.h>
#include <visiontransfer/protocol-sh2-imu-bno080.h> // for sensor constants

#include <iostream>

#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

class DataChannelServiceImpl: public DataChannelServiceBase {
private:
    sockaddr_in serverAddr;
    //
    std::shared_ptr<std::thread> receiverThread;
    unsigned long pollDelay;
    //
    std::shared_ptr<ClientSideDataChannelIMUBNO080> channelBNO080;
    //
    int handleChannel0Message(DataChannelMessage& message, sockaddr_in* sender) override;
    void initiateHandshake();
    void subscribeAll();
    void unsubscribeAll();
    void receiverRoutine();
public:
    bool threadRunning;
    std::vector<DataChannelInfo> channelsAvailable;
    std::map<DataChannel::Type, std::set<DataChannel::ID>> channelsAvailableByType;
public:
    DataChannelServiceImpl(DeviceInfo deviceInfo);
    DataChannelServiceImpl(const char* ipAddr);
    virtual ~DataChannelServiceImpl() { }
    void launch(unsigned long pollDelayUSec);
public:
    // High-level data channels API
    TimestampedQuaternion getLastRotationQuaternion() {
        return channelBNO080->lastRotationQuaternion;
    }
    std::vector<TimestampedQuaternion> getRotationQuaternionSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
        return channelBNO080->ringbufRotationQuaternion.popBetweenTimes(fromSec, fromUSec, untilSec, untilUSec);
    }

    TimestampedVector getLastSensorVector(int idx) {
        return channelBNO080->lastXYZ[idx - 1];
    }
    std::vector<TimestampedVector> getSensorVectorSeries(int idx, int fromSec, int fromUSec, int untilSec, int untilUSec) {
        return channelBNO080->ringbufXYZ[idx - 1].popBetweenTimes(fromSec, fromUSec, untilSec, untilUSec);
    }

    TimestampedScalar getLastSensorScalar(int idx) {
        return channelBNO080->lastScalar[idx - 0x0a];
    }
    std::vector<TimestampedScalar> getSensorScalarSeries(int idx, int fromSec, int fromUSec, int untilSec, int untilUSec) {
        return channelBNO080->ringbufScalar[idx - 0x0a].popBetweenTimes(fromSec, fromUSec, untilSec, untilUSec);
    }
};

} // internal namespace

class DataChannelService::Pimpl {
public:
    std::shared_ptr<internal::DataChannelServiceImpl> impl;
    Pimpl(DeviceInfo deviceInfo) {
        impl = std::make_shared<internal::DataChannelServiceImpl>(deviceInfo);
    }
    Pimpl(const char* ipAddress) {
        impl = std::make_shared<internal::DataChannelServiceImpl>(ipAddress);
    }
};

void internal::DataChannelServiceImpl::receiverRoutine() {
    threadRunning = true;
    while (threadRunning) {
        process();
        std::this_thread::sleep_for(std::chrono::microseconds(pollDelay));
    }
}

void internal::DataChannelServiceImpl::launch(unsigned long pollDelayUSec) {
    // Prepare our receivers (all supported channels aside from service channel 0)
    channelBNO080 = std::make_shared<ClientSideDataChannelIMUBNO080>();
    registerChannel(channelBNO080);
    // Prepare our poll thread
    pollDelay = pollDelayUSec;
    receiverThread = std::make_shared<std::thread>(std::bind(&internal::DataChannelServiceImpl::receiverRoutine, this));
    receiverThread->detach();
    // Say hello to the device to get a channel advertisement
    initiateHandshake();
}


void internal::DataChannelServiceImpl::initiateHandshake() {
    uint16_t cmd = htons((uint16_t) DataChannelControlCommands::CTLRequestAdvertisement);
    sendDataIsolatedPacket((DataChannel::ID) 0x00, DataChannel::Types::CONTROL, (unsigned char*) &cmd, sizeof(cmd), &serverAddr);
}

void internal::DataChannelServiceImpl::subscribeAll() {
    unsigned char data[1024];
    int len = DataChannelControlUtil::packSubscriptionMessage(data, 1024, DataChannelControlCommands::CTLRequestSubscriptions, {0});
    sendDataIsolatedPacket((DataChannel::ID) 0x00, DataChannel::Types::CONTROL, data, len, &serverAddr);
}

void internal::DataChannelServiceImpl::unsubscribeAll() {
    unsigned char data[1024];
    int len = DataChannelControlUtil::packSubscriptionMessage(data, 1024, DataChannelControlCommands::CTLRequestUnsubscriptions, {0});
    sendDataIsolatedPacket((DataChannel::ID) 0x00, DataChannel::Types::CONTROL, data, len, &serverAddr);
}

int internal::DataChannelServiceImpl::handleChannel0Message(DataChannelMessage& message, sockaddr_in* sender) {
    auto cmd = DataChannelControlUtil::getCommand(message.payload, message.header.payloadSize);
    switch (cmd) {
        case DataChannelControlCommands::CTLProvideAdvertisement: {
                // Update the available channels lists for run-time checks etc.
                channelsAvailable = DataChannelControlUtil::unpackAdvertisementMessage(message.payload, message.header.payloadSize);
                for (auto& dci: channelsAvailable) {
                    channelsAvailableByType[dci.getChannelType()].insert(dci.getChannelID());
                }
                // Automatic subscribeAll is suitable for now
                subscribeAll();
                break;
            }
        case DataChannelControlCommands::CTLProvideSubscriptions: {
                break;
            }
        default: {
                break;
            }
    }
    return 1;
}

internal::DataChannelServiceImpl::DataChannelServiceImpl(DeviceInfo deviceInfo)
: DataChannelServiceImpl::DataChannelServiceImpl(deviceInfo.getIpAddress().c_str())
{}

internal::DataChannelServiceImpl::DataChannelServiceImpl(const char* ipAddress)
: DataChannelServiceBase(), threadRunning(false) {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(InternalInformation::DATACHANNELSERVICE_PORT);
    auto result = inet_addr(ipAddress);
    if (result == INADDR_NONE) {
        throw std::runtime_error("Failed to set address for DataChannelService");
    }
    serverAddr.sin_addr.s_addr = result;
    //
    //if (!inet_pton(AF_INET, deviceInfo.getIpAddress().c_str(), &(serverAddr.sin_addr))) {
    //    throw std::runtime_error("Failed to set address for DataChannelService");
    //}
}


DataChannelService::DataChannelService(DeviceInfo deviceInfo, unsigned long pollDelayUSec) {
    pimpl = new DataChannelService::Pimpl(deviceInfo);
    pimpl->impl->launch(pollDelayUSec);
}

DataChannelService::DataChannelService(const char* ipAddress, unsigned long pollDelayUSec) {
    pimpl = new DataChannelService::Pimpl(ipAddress);
    pimpl->impl->launch(pollDelayUSec);
}


DataChannelService::~DataChannelService() {
    pimpl->impl->threadRunning = false;
    delete pimpl;
}

bool DataChannelService::imuAvailable() {
    return pimpl->impl->channelsAvailableByType.count(DataChannel::Types::BNO080);
}


// High-level IMU accessors (C++-98 compatible signatures)

// For devices not providing IMU data, these return placeholder defaults
TimestampedQuaternion DataChannelService::imuGetRotationQuaternion() {
    return pimpl->impl->getLastRotationQuaternion();
}
std::vector<TimestampedQuaternion> DataChannelService::imuGetRotationQuaternionSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getRotationQuaternionSeries(fromSec, fromUSec, untilSec, untilUSec);
}

TimestampedVector DataChannelService::imuGetAcceleration() {
    return pimpl->impl->getLastSensorVector(SH2Constants::SENSOR_ACCELEROMETER);
}
std::vector<TimestampedVector> DataChannelService::imuGetAccelerationSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getSensorVectorSeries(SH2Constants::SENSOR_ACCELEROMETER, fromSec, fromUSec, untilSec, untilUSec);
}

TimestampedVector DataChannelService::imuGetGyroscope() {
    return pimpl->impl->getLastSensorVector(SH2Constants::SENSOR_GYROSCOPE);
}
std::vector<TimestampedVector> DataChannelService::imuGetGyroscopeSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getSensorVectorSeries(SH2Constants::SENSOR_GYROSCOPE, fromSec, fromUSec, untilSec, untilUSec);
}

TimestampedVector DataChannelService::imuGetMagnetometer() {
    return pimpl->impl->getLastSensorVector(SH2Constants::SENSOR_MAGNETOMETER);
}
std::vector<TimestampedVector> DataChannelService::imuGetMagnetometerSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getSensorVectorSeries(SH2Constants::SENSOR_MAGNETOMETER, fromSec, fromUSec, untilSec, untilUSec);
}

TimestampedVector DataChannelService::imuGetLinearAcceleration() {
    return pimpl->impl->getLastSensorVector(SH2Constants::SENSOR_LINEAR_ACCELERATION);
}
std::vector<TimestampedVector> DataChannelService::imuGetLinearAccelerationSeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getSensorVectorSeries(SH2Constants::SENSOR_LINEAR_ACCELERATION, fromSec, fromUSec, untilSec, untilUSec);
}

TimestampedVector DataChannelService::imuGetGravity() {
    return pimpl->impl->getLastSensorVector(SH2Constants::SENSOR_GRAVITY);
}
std::vector<TimestampedVector> DataChannelService::imuGetGravitySeries(int fromSec, int fromUSec, int untilSec, int untilUSec) {
    return pimpl->impl->getSensorVectorSeries(SH2Constants::SENSOR_GRAVITY, fromSec, fromUSec, untilSec, untilUSec);
}

} // visiontransfer namespace

