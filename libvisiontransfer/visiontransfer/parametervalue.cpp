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

#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <set>

#include <iostream>

#include <visiontransfer/parametervalue.h>
#include <visiontransfer/internal/conversionhelpers.h>

namespace visiontransfer {
namespace param {

using namespace internal;

/** A raw castable variant value for parameters, used for several things internally in Parameter */
class ParameterValue::Pimpl {
public:
    Pimpl();
    Pimpl(const Pimpl& other);
    ~Pimpl();
    static void copyData(ParameterValue::Pimpl& dst, const ParameterValue::Pimpl& src);
    Pimpl& setType(ParameterValue::ParameterType t);
    Pimpl& setTensorShape(const std::vector<unsigned int>& shape);
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
    Pimpl& setTensorData(const std::vector<double>& data);
    unsigned int getTensorNumElements() const;
    unsigned int getTensorCurrentDataSize() const;
    ParameterValue::ParameterType getType() const { return type; }
    double& tensorElementAt(unsigned int x);
    double& tensorElementAt(unsigned int y, unsigned int x);
    double& tensorElementAt(unsigned int z, unsigned int y, unsigned int x);

    template<typename T> Pimpl& setValue(T t);
    template<typename T> T getValue() const;
    template<typename T> T getWithDefault(const T& deflt) const { return (type==TYPE_UNDEFINED) ? deflt : getValue<T>(); }

private:
    double numVal;
    std::string stringVal;
    unsigned int tensorNumElements; // quick access to number of elements
    std::vector<unsigned int> tensorShape;
    std::vector<double> tensorData;

    ParameterValue::ParameterType type;

    /// Parameters of TYPE_SAFESTRING enforce a safe character whitelist and max length
    static std::string sanitizeString(const std::string& s, unsigned int maxLength=4096);
};

//static
std::string ParameterValue::Pimpl::sanitizeString(const std::string& s, unsigned int maxLength /* = 4096 */ ) {
    std::ostringstream ss;
    const std::string whitelist("-+_,.:@/ "); // plus all alnums
    unsigned int len = 0;
    for (const char c: s) {
        if (std::isalnum(c) || whitelist.find(c) != std::string::npos) {
            ss << c;
        } else {
            ss << ' ';
        }
        if (++len > maxLength) break; // Trim excessively long safestrings
    }
    return ss.str();
}
//static
void ParameterValue::Pimpl::copyData(ParameterValue::Pimpl& dst, const ParameterValue::Pimpl& src) {
    dst.numVal = src.numVal;
    dst.stringVal = src.stringVal;
    dst.tensorNumElements = src.tensorNumElements;
    dst.tensorShape = src.tensorShape;
    dst.tensorData = src.tensorData;
    dst.type = src.type;
}

ParameterValue::Pimpl::Pimpl()
: numVal(0.0), type(TYPE_UNDEFINED)
{
}
ParameterValue::Pimpl::Pimpl(const ParameterValue::Pimpl& other)
{
    copyData(*this, other);
}
ParameterValue::Pimpl::~Pimpl()
{
}

ParameterValue::Pimpl& ParameterValue::Pimpl::setType(ParameterValue::ParameterType t) {
    type = t;
    return *this;
};

ParameterValue::Pimpl& ParameterValue::Pimpl::setTensorShape(const std::vector<unsigned int>& shape) {
    unsigned int dims = (unsigned int) shape.size();
    if (dims==0) {
        throw std::runtime_error("Cannot create a zero-dimensional tensor");
    }
    int elems = 1;
    for (unsigned int i=0; i<dims; ++i) {
        elems *= shape[i];
    }
    if (elems==0) {
        throw std::runtime_error("Cannot create a tensor with effective size 0");
    }
    tensorNumElements = elems;
    tensorShape = shape;
    tensorData.reserve(elems);
    return *this;
}

bool ParameterValue::Pimpl::isDefined() const {
    return (type!=TYPE_UNDEFINED);
}
bool ParameterValue::Pimpl::isUndefined() const {
    return (type==TYPE_UNDEFINED);
}
bool ParameterValue::Pimpl::isTensor() const {
    return type==TYPE_TENSOR;
}
bool ParameterValue::Pimpl::isScalar() const {
    return !isTensor();
}
bool ParameterValue::Pimpl::isCommand() const {
    return type==TYPE_COMMAND;
}

unsigned int ParameterValue::Pimpl::getTensorDimension() const {
    return (unsigned int) tensorShape.size();
}

std::vector<unsigned int> ParameterValue::Pimpl::getTensorShape() const {
    return tensorShape;
}

std::vector<double> ParameterValue::Pimpl::getTensorData() const {
    return tensorData;
}

ParameterValue::Pimpl& ParameterValue::Pimpl::setTensorData(const std::vector<double>& data) {
    if (data.size() != tensorNumElements) throw std::runtime_error("ParameterValue::Pimpl::setTensorData(): wrong number of elements");

    setType(ParameterType::TYPE_TENSOR);
    tensorData = data;
    // Also cache a pre-rendered string version
    std::ostringstream os;
    for (unsigned int i=0; i<getTensorNumElements(); ++i) {
        if (i) os << " ";
        os << ConversionHelpers::anyToString(tensorData[i]);
    }
    stringVal = os.str();
    return *this;
}

unsigned int ParameterValue::Pimpl::getTensorNumElements() const {
    return tensorNumElements;
}

unsigned int ParameterValue::Pimpl::getTensorCurrentDataSize() const {
    return (unsigned int) tensorData.size();
}

double& ParameterValue::Pimpl::tensorElementAt(unsigned int x) {
    // Pure 1-dim support
    //if (tensorShape.size()!=1) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): not a tensor of dimension 1");
    //if (x>=tensorShape[0]) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): access out of bounds");
    // Any-dim support (allow self-addressing)
    if (tensorShape.size()==0) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): not a tensor");
    if (x>=tensorNumElements) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): access out of bounds");
    return tensorData[x];
}
double& ParameterValue::Pimpl::tensorElementAt(unsigned int y, unsigned int x) {
    if (tensorShape.size()!=2) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): not a tensor of dimension 2");
    if (y>=tensorShape[0] || x>=tensorShape[1]) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): access out of bounds");
    return tensorData[y*tensorShape[1] + x];
}
double& ParameterValue::Pimpl::tensorElementAt(unsigned int z, unsigned int y, unsigned int x) {
    if (tensorShape.size()!=3) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): not a tensor of dimension 3");
    if (z>=tensorShape[0] || y>=tensorShape[1] || x>=tensorShape[2]) throw std::runtime_error("ParameterValue::Pimpl::tensorElementAt(): access out of bounds");
    return tensorData[z*tensorShape[1]*tensorShape[2] + y*tensorShape[2] + x];
}


// setters
template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(int t) {
    // always cache string
    switch (this->type) {
        case TYPE_INT:
        case TYPE_DOUBLE:
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
            numVal = t;
            stringVal = ConversionHelpers::anyToString(t);
            break;
        case TYPE_BOOL:
            numVal = (t==0) ? 0 : 1;
            stringVal = (t==0) ? "false" : "true";
            break;
        case TYPE_TENSOR:
            // could in theory accept iff tensor is of exactly one-element size
            throw std::runtime_error("Cannot assign a raw scalar to a tensor parameter");
        case TYPE_UNDEFINED:
            throw std::runtime_error("Cannot assign a value to an undefined parameter");
    }
    return *this;
}

template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(bool t) {
    // always cache string
    switch (this->type) {
        case TYPE_INT:
        case TYPE_DOUBLE:
            numVal = t ? 1 : 0;
            stringVal = ConversionHelpers::anyToString(numVal);
            break;
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
        case TYPE_BOOL:
            numVal = (t==false) ? 0 : 1;
            stringVal = (t==false) ? "false" : "true";
            break;
        case TYPE_TENSOR:
            // could in theory accept iff tensor is of exactly one-element size
            throw std::runtime_error("Cannot assign a raw scalar to a tensor parameter");
        case TYPE_UNDEFINED:
            throw std::runtime_error("Cannot assign a value to an undefined parameter");
    }
    return *this;
}
template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(double t) {
    // always cache string
    switch (this->type) {
        case TYPE_DOUBLE:
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
            numVal = t;
            stringVal = ConversionHelpers::anyToString(t);
            break;
        case TYPE_INT:
            numVal = (int) t;
            stringVal = ConversionHelpers::anyToString((int) t); // lose decimals here
            break;
        case TYPE_BOOL:
            numVal = (t==0) ? 0 : 1;
            stringVal = (t==0) ? "false" : "true";
            break;
        case TYPE_TENSOR:
            // could in theory accept iff tensor is of exactly one-element size
            throw std::runtime_error("Cannot assign a raw scalar to a tensor parameter");
        case TYPE_UNDEFINED:
            throw std::runtime_error("Cannot assign a value to an undefined parameter");
    }
    return *this;
}

template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(const char* t) {
    // always cache string
    switch (this->type) {
        case TYPE_COMMAND:
        case TYPE_SAFESTRING:
            // sanitize!
            stringVal = sanitizeString(t);
            numVal = atof(stringVal.c_str());
            break;
        case TYPE_STRING:
            stringVal = t;
            numVal = atof(t);
            break;
        case TYPE_DOUBLE:
            numVal = atof(t);
            stringVal = ConversionHelpers::anyToString(numVal);
            break;
        case TYPE_INT:
            // Lenient parsing of bools as 0/1; otherwise take the int portion of the string
            if (!std::strncmp("true", t, 4) || !std::strncmp("True", t, 4)) {
                numVal = 1;
            } else if (!std::strncmp("false", t, 5) || !std::strncmp("False", t, 5)) {
                numVal = 0;
            } else {
                numVal = atol(t);
            }
            stringVal = ConversionHelpers::anyToString((int) numVal);
            break;
        case TYPE_BOOL:
            if (!std::strncmp("true", t, 4) || !std::strncmp("True", t, 4)) {
                numVal = 1;
            } else {
                numVal = atol(t)==0 ? 0 : 1;
            }
            stringVal = (numVal==0.0) ? "false" : "true";
            break;
        case TYPE_TENSOR:
            // could in theory accept iff tensor is of exactly one-element size
            throw std::runtime_error("Cannot assign a raw scalar to a tensor parameter");
        case TYPE_UNDEFINED:
            throw std::runtime_error("Cannot assign a value to an undefined parameter");
    }
    return *this;
}

template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(const std::string& t) {
    return setValue<const char*>(t.c_str());
}
template<> VT_EXPORT
ParameterValue::Pimpl& ParameterValue::Pimpl::setValue(std::string t) {
    return setValue<const char*>(t.c_str());
}

// getters
template<> VT_EXPORT
int ParameterValue::Pimpl::getValue() const {
    switch (this->type) {
        case TYPE_INT: case TYPE_DOUBLE: case TYPE_BOOL:
            return (int) numVal;
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
            return (int) numVal; // also ok, since cached
        case TYPE_TENSOR:
            // could also return 0 or a dedicated type exception (or be OK if size ==1 element)
            throw std::runtime_error("Attempted to get tensor parameter as scalar- undefined value");
        case TYPE_UNDEFINED:
        default:
            return 0; // silent default
    }
}
template<> VT_EXPORT
double ParameterValue::Pimpl::getValue() const {
    switch (this->type) {
        case TYPE_INT: case TYPE_DOUBLE: case TYPE_BOOL:
            return (double) numVal;
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
            return (double) numVal; // also ok, since cached
        case TYPE_TENSOR:
            // could also return 0 or a dedicated type exception (or be OK if size ==1 element)
            throw std::runtime_error("Attempted to get tensor parameter as scalar- undefined value");
        case TYPE_UNDEFINED:
        default:
            return 0.0; // silent default
    }
}
template<> VT_EXPORT
bool ParameterValue::Pimpl::getValue() const {
    switch (this->type) {
        case TYPE_INT: case TYPE_DOUBLE: case TYPE_BOOL:
            return (bool) numVal;
        case TYPE_STRING:
        case TYPE_SAFESTRING:
        case TYPE_COMMAND:
            return (bool) numVal; // also ok, since cached
        case TYPE_TENSOR:
            // could also return 0 or a dedicated type exception (or be OK if size ==1 element)
            throw std::runtime_error("Attempted to get tensor parameter as scalar- undefined value");
        case TYPE_UNDEFINED:
        default:
            return false; // silent default
    }
}
template<> VT_EXPORT
std::string ParameterValue::Pimpl::getValue() const {
    switch (this->type) {
        case TYPE_INT: case TYPE_DOUBLE: case TYPE_BOOL:
            // string is pre-rendered
        case TYPE_TENSOR:
            // also pre-rendered (in setTensorData)
        case TYPE_STRING: case TYPE_SAFESTRING: case TYPE_COMMAND:
            return stringVal;
        case TYPE_UNDEFINED:
        default:
            return ""; // silent default
    }
}

// Mainly for literals placed in test code etc - maybe remove?
template<> VT_EXPORT
const char* ParameterValue::Pimpl::getValue() const {
    switch (this->type) {
        case TYPE_INT: case TYPE_DOUBLE: case TYPE_BOOL:
            // string is pre-rendered
        case TYPE_TENSOR:
            // also pre-rendered (in setTensorData)
        case TYPE_STRING: case TYPE_SAFESTRING: case TYPE_COMMAND:
            return stringVal.c_str();
        case TYPE_UNDEFINED:
        default:
            return ""; // silent default
    }
}


//
//
// External (API) class
//
//

ParameterValue::ParameterValue()
: pimpl(new ParameterValue::Pimpl())
{
}

ParameterValue::ParameterValue(const ParameterValue& other)
: pimpl(new ParameterValue::Pimpl())
{
    ParameterValue::Pimpl::copyData(*pimpl, *(other.pimpl));
}

ParameterValue::~ParameterValue() {
    delete pimpl;
}

ParameterValue& ParameterValue::operator=(const ParameterValue& other) {
    ParameterValue::Pimpl::copyData(*pimpl, *(other.pimpl));
    return *this;
}

ParameterValue& ParameterValue::setType(ParameterValue::ParameterType t) {
    pimpl->setType(t);
    return *this;
}

ParameterValue& ParameterValue::setTensorShape(const std::vector<unsigned int>& shape) {
    pimpl->setTensorShape(shape);
    return *this;
}

bool ParameterValue::isDefined() const {
    return pimpl->isDefined();
}

bool ParameterValue::isUndefined() const {
    return pimpl->isUndefined();
}
bool ParameterValue::isTensor() const {
    return pimpl->isTensor();
}
bool ParameterValue::isScalar() const {
    return pimpl->isScalar();
}
bool ParameterValue::isCommand() const {
    return pimpl->isCommand();
}
unsigned int ParameterValue::getTensorDimension() const {
    return pimpl->getTensorDimension();
}
std::vector<unsigned int> ParameterValue::getTensorShape() const {
    return pimpl->getTensorShape();
}
/// Return a copy of the internal tensor data
std::vector<double> ParameterValue::getTensorData() const {
    return pimpl->getTensorData();
}
/// Return a reference to the internal tensor data (caution)
std::vector<double>& ParameterValue::getTensorDataReference() {
    return pimpl->getTensorDataReference();
}
ParameterValue& ParameterValue::setTensorData(const std::vector<double>& data) {
    pimpl->setTensorData(data);
    return *this;
}
unsigned int ParameterValue::getTensorNumElements() const {
    return pimpl->getTensorNumElements();
}
unsigned int ParameterValue::getTensorCurrentDataSize() const {
    return pimpl->getTensorCurrentDataSize();
}
ParameterValue::ParameterType ParameterValue::getType() const {
    return pimpl->getType();
}
double& ParameterValue::tensorElementAt(unsigned int x) {
    return pimpl->tensorElementAt(x);
}
double& ParameterValue::tensorElementAt(unsigned int y, unsigned int x) {
    return pimpl->tensorElementAt(y, x);
}
double& ParameterValue::tensorElementAt(unsigned int z, unsigned int y, unsigned int x) {
    return pimpl->tensorElementAt(z, y, x);
}

// getters

template<> VT_EXPORT
int ParameterValue::getValue() const {
    return pimpl->getValue<int>();
}
template<> VT_EXPORT
bool ParameterValue::getValue() const {
    return pimpl->getValue<bool>();
}
template<> VT_EXPORT
double ParameterValue::getValue() const {
    return pimpl->getValue<double>();
}
template<> VT_EXPORT
std::string ParameterValue::getValue() const {
    return pimpl->getValue<std::string>();
}
template<> VT_EXPORT
const char* ParameterValue::getValue() const {
    return pimpl->getValue<const char*>();
}

// setters
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(int t) {
    pimpl->setValue<int>(t);
    return *this;
}
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(bool t) {
    pimpl->setValue<bool>(t);
    return *this;
}
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(double t) {
    pimpl->setValue<double>(t);
    return *this;
}
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(const char* t) {
    pimpl->setValue<const char*>(t);
    return *this;
}
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(const std::string& t) {
    return setValue<const char*>(t.c_str());
}
template<> VT_EXPORT
ParameterValue& ParameterValue::setValue(std::string t) {
    return setValue<const char*>(t.c_str());
}

} // namespace param
} // namespace visiontransfer

