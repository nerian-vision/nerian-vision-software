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

#ifndef VISIONTRANSFER_PARAMETERVALUE_H
#define VISIONTRANSFER_PARAMETERVALUE_H

#include <string>
#include <vector>
#include <map>

#include <cstring>
#include <sstream>
#include <iomanip>
#include <limits>
#include <memory>

#include <visiontransfer/common.h>

namespace visiontransfer {
namespace param {


/** A raw castable variant value for parameters, used for several things internally in Parameter */
class VT_EXPORT ParameterValue {
public:
    enum ParameterType {
        TYPE_INT,
        TYPE_DOUBLE,
        TYPE_BOOL,
        TYPE_STRING,
        TYPE_SAFESTRING,
        TYPE_TENSOR,
        TYPE_COMMAND,
        TYPE_UNDEFINED
    };

    ParameterValue();
    ParameterValue& setType(ParameterType t);
    ParameterValue& setTensorShape(const std::vector<unsigned int>& shape);
    bool isDefined() const;
    bool isUndefined() const;
    bool isTensor() const;
    bool isScalar() const;
    bool isCommand() const;
    unsigned int getTensorDimension() const;
    std::vector<unsigned int> getTensorShape() const;
    /// Return a copy of the internal tensor data
    std::vector<double> getTensorData() const;
    /// Return a reference to the internal tensor data (caution)
    std::vector<double>& getTensorDataReference() { return tensorData; };
    ParameterValue& setTensorData(const std::vector<double>& data);
    unsigned int getTensorNumElements() const;
    unsigned int getTensorCurrentDataSize() const;
    ParameterType getType() const { return type; }
    double& tensorElementAt(unsigned int x);
    double& tensorElementAt(unsigned int y, unsigned int x);
    double& tensorElementAt(unsigned int z, unsigned int y, unsigned int x);

    template<typename T> ParameterValue& setValue(T t);
    template<typename T> T getValue() const;
    template<typename T> T getWithDefault(const T& deflt) const { return (type==TYPE_UNDEFINED) ? deflt : getValue<T>(); }

private:
    double numVal;
    std::string stringVal;
    unsigned int tensorNumElements; // quick access to number of elements
    std::vector<unsigned int> tensorShape;
    std::vector<double> tensorData;

    ParameterType type;

    /// Parameters of TYPE_SAFESTRING enforce a safe character whitelist and max length
    std::string sanitizeString(const std::string& s, unsigned int maxLength=4096);
};

} // namespace param
} // namespace visiontransfer

#endif




