/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#include "visiontransfer/deviceparameters.h"
#include "visiontransfer/parametertransfer.h"
#include "visiontransfer/exceptions.h"
#include "visiontransfer/common.h"

using namespace visiontransfer;
using namespace visiontransfer::internal;
using namespace visiontransfer::param;

namespace visiontransfer {

/*************** Pimpl class containing all private members ***********/

class DeviceParameters::Pimpl {
public:
    Pimpl(const DeviceInfo& device);
    Pimpl(const char* address, const char* service);

    template<typename T>
    void writeParameter(const char* id, const T& value) {
        paramTrans.writeParameterTransactionGuarded(id, value);
    }

    int readIntParameter(const char* id);
    double readDoubleParameter(const char* id);
    bool readBoolParameter(const char* id);

    void writeIntParameter(const char* id, int value);
    void writeDoubleParameter(const char* id, double value);
    void writeBoolParameter(const char* id, bool value);
    void writeBoolParameterUnguarded(const char* id, bool value);

    std::map<std::string, ParameterInfo> getAllParameters();

    bool hasParameter(const std::string& name) const;

    Parameter const& getParameter(const std::string& name) const;

    ParameterSet const& getParameterSet() const;

    void setParameterUpdateCallback(std::function<void(const std::string& uid)> callback);

    void transactionStartQueue();
    void transactionCommitQueue(int maxWaitMilliseconds);

    void persistParameters(const std::vector<std::string>& uids, bool blocking=true);

private:
    std::map<std::string, ParameterInfo> serverSideEnumeration;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template<typename T>
    void setNamedParameterInternal(const std::string& name, T value);
#endif

    ParameterTransfer paramTrans;
};

/******************** Stubs for all public members ********************/

DeviceParameters::DeviceParameters(const DeviceInfo& device):
    pimpl(new Pimpl(device)) {
    // All initialization in the pimpl class
}

DeviceParameters::DeviceParameters(const char* address, const char* service):
    pimpl(new Pimpl(address, service)) {
    // All initialization in the pimpl class
}

DeviceParameters::~DeviceParameters() {
    delete pimpl;
}

int DeviceParameters::readIntParameter(const char* id) {
    return pimpl->readIntParameter(id);
}

double DeviceParameters::readDoubleParameter(const char* id) {
    return pimpl->readDoubleParameter(id);
}

bool DeviceParameters::readBoolParameter(const char* id) {
    return pimpl->readBoolParameter(id);
}

void DeviceParameters::writeIntParameter(const char* id, int value) {
    pimpl->writeIntParameter(id, value);
}

void DeviceParameters::writeDoubleParameter(const char* id, double value) {
    pimpl->writeDoubleParameter(id, value);
}

void DeviceParameters::writeBoolParameter(const char* id, bool value) {
    pimpl->writeBoolParameter(id, value);
}

void DeviceParameters::writeBoolParameterUnguarded(const char* id, bool value) {
    pimpl->writeBoolParameterUnguarded(id, value);
}

std::map<std::string, ParameterInfo> DeviceParameters::getAllParameters()
{
    return pimpl->getAllParameters();
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// Deprecated versions
template<>
void VT_EXPORT DeviceParameters::setNamedParameter(const std::string& name, double value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setNamedParameter(const std::string& name, int value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setNamedParameter(const std::string& name, bool value) {
    pimpl->writeParameter(name.c_str(), value);
}
// New versions (extensible for access for other types)
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, double value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, int value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, bool value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, const std::string& value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, std::string value) {
    pimpl->writeParameter(name.c_str(), value);
}
template<>
void VT_EXPORT DeviceParameters::setParameter(const std::string& name, std::vector<double> tensorValues) {
    std::stringstream ss;
    int i=0;
    for (const auto& v: tensorValues) {
        if (i++) ss << " ";
        ss << v;
    }
    pimpl->writeParameter(name.c_str(), ss.str());
}

template<>
int VT_EXPORT DeviceParameters::getNamedParameter(const std::string& name) {
    return pimpl->readIntParameter(name.c_str());
}
template<>
double VT_EXPORT DeviceParameters::getNamedParameter(const std::string& name) {
    return pimpl->readDoubleParameter(name.c_str());
}
template<>
bool VT_EXPORT DeviceParameters::getNamedParameter(const std::string& name) {
    return pimpl->readBoolParameter(name.c_str());
}
#endif

#if VISIONTRANSFER_CPLUSPLUS_VERSION >= 201103L
bool DeviceParameters::hasParameter(const std::string& name) const {
    return pimpl->hasParameter(name);
}

Parameter DeviceParameters::getParameter(const std::string& name) const {
    return pimpl->getParameter(name); // copied here
}

ParameterSet DeviceParameters::getParameterSet() const {
    return pimpl->getParameterSet(); // copied here
}

void DeviceParameters::setParameterUpdateCallback(std::function<void(const std::string& uid)> callback) {
    pimpl->setParameterUpdateCallback(callback);
}

DeviceParameters::TransactionLock::TransactionLock(DeviceParameters::Pimpl* pimpl)
: pimpl(pimpl) {
    pimpl->transactionStartQueue();
}

DeviceParameters::TransactionLock::~TransactionLock() {
    pimpl->transactionCommitQueue(-1); // non-blocking
}

void DeviceParameters::TransactionLock::commitAndWait(int waitMaxMilliseconds) {
    pimpl->transactionCommitQueue(waitMaxMilliseconds);
}

std::unique_ptr<DeviceParameters::TransactionLock> DeviceParameters::transactionLock() {
    return std::make_unique<DeviceParameters::TransactionLock>(pimpl);
}

void DeviceParameters::saveParameter(const char* uid, bool blockingCall) {
    pimpl->persistParameters({uid}, blockingCall);
}
void DeviceParameters::saveParameter(const std::string& uid, bool blockingCall) {
    pimpl->persistParameters({uid}, blockingCall);
}
void DeviceParameters::saveParameter(const visiontransfer::param::Parameter& param, bool blockingCall) {
    pimpl->persistParameters({param.getUid()}, blockingCall);
}
void DeviceParameters::saveParameters(const std::vector<std::string>& uids, bool blockingCall) {
    pimpl->persistParameters(uids, blockingCall);
}
void DeviceParameters::saveParameters(const std::set<std::string>& uids, bool blockingCall) {
    std::vector<std::string> uidvec;
    for (auto const& u: uids) uidvec.push_back(u);
    pimpl->persistParameters(uidvec, blockingCall);
}
void DeviceParameters::saveParameters(std::initializer_list<std::string> uids, bool blockingCall) {
    std::vector<std::string> uidvec;
    for (auto const& u: uids) uidvec.push_back(u);
    pimpl->persistParameters(uidvec, blockingCall);
}

#endif

/******************** Implementation in pimpl class *******************/

DeviceParameters::Pimpl::Pimpl(const char* address, const char* service)
    : paramTrans(address, service) {
}

DeviceParameters::Pimpl::Pimpl(const DeviceInfo& device)
    : paramTrans(device.getIpAddress().c_str(), "7683") {
}

int DeviceParameters::Pimpl::readIntParameter(const char* id) {
    return paramTrans.readIntParameter(id);
}

double DeviceParameters::Pimpl::readDoubleParameter(const char* id) {
    return paramTrans.readDoubleParameter(id);
}

bool DeviceParameters::Pimpl::readBoolParameter(const char* id) {
    return paramTrans.readBoolParameter(id);
}

void DeviceParameters::Pimpl::writeIntParameter(const char* id, int value) {
    paramTrans.writeParameterTransactionGuarded(id, value);
}

void DeviceParameters::Pimpl::writeDoubleParameter(const char* id, double value) {
    paramTrans.writeParameterTransactionGuarded(id, value);
}

void DeviceParameters::Pimpl::writeBoolParameter(const char* id, bool value) {
    paramTrans.writeParameterTransactionGuarded(id, value);
}

void DeviceParameters::Pimpl::writeBoolParameterUnguarded(const char* id, bool value) {
    paramTrans.writeParameterTransactionUnguarded(id, value);
}

std::map<std::string, ParameterInfo> DeviceParameters::Pimpl::getAllParameters() {
    serverSideEnumeration = paramTrans.getAllParameters();
    return serverSideEnumeration;
}

bool DeviceParameters::Pimpl::hasParameter(const std::string& name) const {
    auto const& paramSet = paramTrans.getParameterSet();
    return paramSet.count(name) != 0;
}

Parameter const& DeviceParameters::Pimpl::getParameter(const std::string& name) const {
    auto const& paramSet = paramTrans.getParameterSet();
    if (paramSet.count(name)) {
        return paramSet.at(name);
    } else {
        throw ParameterException(std::string("Invalid or inaccessible parameter name: ") + name);
    }
}

ParameterSet const& DeviceParameters::Pimpl::getParameterSet() const {
    return paramTrans.getParameterSet();
}

void DeviceParameters::Pimpl::setParameterUpdateCallback(std::function<void(const std::string& uid)> callback) {
    paramTrans.setParameterUpdateCallback(callback);
}

void DeviceParameters::Pimpl::transactionStartQueue() {
    paramTrans.transactionStartQueue();
}

void DeviceParameters::Pimpl::transactionCommitQueue(int maxWaitMilliseconds) {
    paramTrans.transactionCommitQueue(maxWaitMilliseconds);
}

void DeviceParameters::Pimpl::persistParameters(const std::vector<std::string>& uids, bool blocking) {
    paramTrans.persistParameters(uids, blocking);
}

} // namespace

