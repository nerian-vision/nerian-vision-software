/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
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

#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <set>

#include <iostream>

#include <visiontransfer/parameterset.h>

namespace visiontransfer {
namespace param {


Parameter& ParameterSet::setOrCreateSimpleScalar(const std::string& uid, int value) {
    auto it = find(uid);
    if (it==end()) {
        Parameter par(uid);
        par.setType(ParameterValue::TYPE_INT).setCurrent(value);
        operator[](uid) = par;
    } else {
        if (it->second.isTensor() || it->second.isCommand()) {
            throw std::runtime_error("setOrCreateSimpleScalar(): refusing to overwrite a Tensor or Command parameter");
        }
        operator[](uid).setCurrent(value);
    }
    return operator[](uid);
}
Parameter& ParameterSet::setOrCreateSimpleScalar(const std::string& uid, bool value) {
    auto it = find(uid);
    if (it==end()) {
        Parameter par(uid);
        par.setType(ParameterValue::TYPE_BOOL).setCurrent(value);
        operator[](uid) = par;
    } else {
        if (it->second.isTensor() || it->second.isCommand()) {
            throw std::runtime_error("setOrCreateSimpleScalar(): refusing to overwrite a Tensor or Command parameter");
        }
        operator[](uid).setCurrent(value);
    }
    return operator[](uid);
}
Parameter& ParameterSet::setOrCreateSimpleScalar(const std::string& uid, double value) {
    auto it = find(uid);
    if (it==end()) {
        Parameter par(uid);
        par.setType(ParameterValue::TYPE_DOUBLE).setCurrent(value);
        operator[](uid) = par;
    } else {
        if (it->second.isTensor() || it->second.isCommand()) {
            throw std::runtime_error("setOrCreateSimpleScalar(): refusing to overwrite a Tensor or Command parameter");
        }
        operator[](uid).setCurrent(value);
    }
    return operator[](uid);
}
Parameter& ParameterSet::setOrCreateSimpleScalar(const std::string& uid, const std::string& value) {
    auto it = find(uid);
    if (it==end()) {
        Parameter par(uid);
        par.setType(ParameterValue::TYPE_STRING).setCurrent(value);
        operator[](uid) = par;
    } else {
        if (it->second.isTensor() || it->second.isCommand()) {
            throw std::runtime_error("setOrCreateSimpleScalar(): refusing to overwrite a Tensor or Command parameter");
        }
        operator[](uid).setCurrent(value);
    }
    return operator[](uid);
}

} // namespace param
} // namespace visiontransfer

