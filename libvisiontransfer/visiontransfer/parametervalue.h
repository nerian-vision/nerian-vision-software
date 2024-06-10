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
class ParameterValue {

private:
    // We (mostly) follow the pimpl idiom here
    class Pimpl;
    Pimpl* pimpl;

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

   VT_EXPORT ParameterValue();
   VT_EXPORT ParameterValue(const ParameterValue& other);
   VT_EXPORT ~ParameterValue();
   VT_EXPORT ParameterValue& operator= (const ParameterValue& other);
   VT_EXPORT ParameterValue& setType(ParameterType t);
   VT_EXPORT ParameterValue& setTensorShape(const std::vector<unsigned int>& shape);
   VT_EXPORT bool isDefined() const;
   VT_EXPORT bool isUndefined() const;
   VT_EXPORT bool isTensor() const;
   VT_EXPORT bool isScalar() const;
   VT_EXPORT bool isCommand() const;
   VT_EXPORT unsigned int getTensorDimension() const;
   VT_EXPORT std::vector<unsigned int> getTensorShape() const;
   /// Return a copy of the internal tensor data
   VT_EXPORT std::vector<double> getTensorData() const;
   /// Return a reference to the internal tensor data (caution)
   VT_EXPORT std::vector<double>& getTensorDataReference();
   VT_EXPORT ParameterValue& setTensorData(const std::vector<double>& data);
   VT_EXPORT unsigned int getTensorNumElements() const;
   VT_EXPORT unsigned int getTensorCurrentDataSize() const;
   VT_EXPORT ParameterType getType() const;
   VT_EXPORT double& tensorElementAt(unsigned int x);
   VT_EXPORT double& tensorElementAt(unsigned int y, unsigned int x);
   VT_EXPORT double& tensorElementAt(unsigned int z, unsigned int y, unsigned int x);

   template<typename T> VT_EXPORT ParameterValue& setValue(T t);
   template<typename T> VT_EXPORT T getValue() const;
   template<typename T> VT_EXPORT T getWithDefault(const T& deflt) const;

};

} // namespace param
} // namespace visiontransfer

#endif




