/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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

#ifndef VISIONTRANSFER_PARAMETERSET_H
#define VISIONTRANSFER_PARAMETERSET_H

#include <string>
#include <vector>
#include <map>

#include <cstring>
#include <sstream>
#include <iomanip>
#include <limits>
#include <memory>

#include <visiontransfer/common.h>
#include <visiontransfer/parameter.h>

namespace visiontransfer {
namespace param {

/** ParameterSet is a map from UIDs to Parameters with some extra functionality. */
class ParameterSet: public std::map<std::string, Parameter> {
private:
    // We (mostly) follow the pimpl idiom here
    class Pimpl;
    Pimpl* pimpl; // Note: reserved for now for forward compatibility

public:
    typedef std::shared_ptr<ParameterSet> ptr;
    /// Checked parameter getter for clients to avoid instantiation of incomplete ones
    VT_EXPORT Parameter& get(const std::string& uid) {
        auto it = find(uid);
        if (it==end()) throw std::runtime_error(std::string("Attempted to get nonexistent parameter ") + uid);
        return it->second;
    }
    VT_EXPORT bool add(const Parameter& param) { operator[](param.getUid()) = param; return true; }
    /// Convenience function for safe bulk parameter access (fallback for invalid UIDs). Will return any default value if UID exists but current value unset.
    template<typename T>
    VT_EXPORT T getCurrentOrFallback(const std::string& key, T&& fallback) {
        auto it = find(key);
        if (it!=end()) return it->second.getCurrent<T>();
        else return (T) fallback;
    }
    /// Convenience function for safe bulk parameter access (throws for invalid UIDs). Will return any default value if UID exists but current value unset.
    template<typename T>
    VT_EXPORT T getCurrent(const std::string& key) {
        auto it = find(key);
        if (it!=end()) return it->second.getCurrent<T>();
        else throw std::runtime_error(std::string("Parameter not found in the parameter set: ") + key);
    }
    // Convenience functions for quickly adding (internal) parameter values; scalars with no initial metadata
    VT_EXPORT Parameter& setOrCreateSimpleScalar(const std::string& uid, int value);
    VT_EXPORT Parameter& setOrCreateSimpleScalar(const std::string& uid, bool value);
    VT_EXPORT Parameter& setOrCreateSimpleScalar(const std::string& uid, double value);
    VT_EXPORT Parameter& setOrCreateSimpleScalar(const std::string& uid, const std::string& value);
};

} // namespace param
} // namespace visiontransfer

#endif

