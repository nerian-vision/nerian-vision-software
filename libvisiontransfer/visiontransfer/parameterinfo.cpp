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

#include "visiontransfer/parameterinfo.h"
#include "visiontransfer/exceptions.h"

namespace visiontransfer {

class ParameterInfo::Pimpl {
public:
    Pimpl(): type(ParameterInfo::TYPE_INT), value({0}), min({0}), max({0}), inc({0}) { }
    template<typename T> void set(const std::string& name, bool writeable,
        T value, T min, T max, T inc);
    inline std::string getName() const { return name; }
    inline ParameterType getType() const { return type; }
    inline bool isWriteable() const { return writeable; }
    template<typename T> T getTypedValue(const ParameterValue& val) const;
    template<typename T> T getValue() const { return getTypedValue<T>(value); }
    template<typename T> T getMin() const { return getTypedValue<T>(min); }
    template<typename T> T getMax() const { return getTypedValue<T>(max); }
    template<typename T> T getInc() const { return getTypedValue<T>(inc); }
private:
    std::string name;
    ParameterType type;
    bool writeable;
    ParameterValue value;
    ParameterValue min;
    ParameterValue max;
    ParameterValue inc;
};

// ParameterInfo, for abstracted enumerations of parameters

ParameterInfo::ParameterInfo()
{
    pimpl = new ParameterInfo::Pimpl();
}

template<> void ParameterInfo::Pimpl::set(const std::string& name_, bool writeable_, int value_, int min_, int max_, int inc_)
{
    this->name = name_;
    this->type = ParameterInfo::TYPE_INT;
    this->writeable = writeable_;
    this->value.intVal = value_;
    this->min.intVal = min_;
    this->max.intVal = max_;
    this->inc.intVal = inc_;
}
template<> void ParameterInfo::Pimpl::set(const std::string& name_, bool writeable_, double value_, double min_, double max_, double inc_)
{
    this->name = name_;
    this->type = ParameterInfo::TYPE_DOUBLE;
    this->writeable = writeable_;
    this->value.doubleVal = value_;
    this->min.doubleVal = min_;
    this->max.doubleVal = max_;
    this->inc.doubleVal = inc_;
}
template<> void ParameterInfo::Pimpl::set(const std::string& name_, bool writeable_, bool value_, bool min_, bool max_, bool inc_)
{
    this->name = name_;
    this->type = ParameterInfo::TYPE_BOOL;
    this->writeable = writeable_;
    this->value.boolVal = value_;
    this->min.boolVal = min_;
    this->max.boolVal = max_;
    this->inc.boolVal = inc_;
}

ParameterInfo ParameterInfo::fromInt(const std::string& name_, bool writeable_,
        int value_, int min_, int max_, int inc_) {
    ParameterInfo pi;
    pi.pimpl->set<int>(name_, writeable_, value_, min_, max_, inc_);
    return pi;
}

ParameterInfo ParameterInfo::fromDouble(const std::string& name_, bool writeable_,
        double value_, double min_, double max_, double inc_) {
    ParameterInfo pi;
    pi.pimpl->set<double>(name_, writeable_, value_, min_, max_, inc_);
    return pi;
}

ParameterInfo ParameterInfo::fromBool(const std::string& name_, bool writeable_, bool value_) {
    ParameterInfo pi;
    pi.pimpl->set<bool>(name_, writeable_, value_, 0, 1, 1);
    return pi;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template<> int ParameterInfo::Pimpl::getTypedValue(const ParameterInfo::ParameterValue& val) const {
    switch (type) {
        case ParameterInfo::TYPE_INT: {
                return val.intVal;
            }
        case ParameterInfo::TYPE_BOOL: {
                return (int) val.boolVal;
            }
        case ParameterInfo::TYPE_DOUBLE: {
                return (int) val.doubleVal;
            }
    }
    throw ParameterException("Unexpected parameter type");
}

template<> double ParameterInfo::Pimpl::getTypedValue(const ParameterInfo::ParameterValue& val) const {
    switch (type) {
        case ParameterInfo::TYPE_DOUBLE: {
                return val.doubleVal;
            }
        case ParameterInfo::TYPE_INT: {
                return (double) val.intVal;
            }
        case ParameterInfo::TYPE_BOOL: {
                return val.boolVal?1.0:0.0;
            }
    }
    throw ParameterException("Unexpected parameter type");
}

template<> bool ParameterInfo::Pimpl::getTypedValue(const ParameterInfo::ParameterValue& val) const {
    switch (type) {
        case ParameterInfo::TYPE_BOOL: {
                return val.boolVal;
            }
        case ParameterInfo::TYPE_DOUBLE: {
                return val.doubleVal != 0.0;
            }
        case ParameterInfo::TYPE_INT: {
                return val.intVal != 0;
            }
    }
    throw ParameterException("Unexpected parameter type");
}
#endif // DOXYGEN_SHOULD_SKIP_THIS

std::string ParameterInfo::getName() const { return pimpl->getName(); }
ParameterInfo::ParameterType ParameterInfo::getType() const { return pimpl->getType(); }
bool ParameterInfo::isWriteable() const { return pimpl->isWriteable(); }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template<> VT_EXPORT int ParameterInfo::getValue() const { return pimpl->getValue<int>(); }
template<> VT_EXPORT double ParameterInfo::getValue() const { return pimpl->getValue<double>(); }
template<> VT_EXPORT bool ParameterInfo::getValue() const { return pimpl->getValue<bool>(); }
template<> VT_EXPORT int ParameterInfo::getMin() const { return pimpl->getMin<int>(); }
template<> VT_EXPORT double ParameterInfo::getMin() const { return pimpl->getMin<double>(); }
template<> VT_EXPORT bool ParameterInfo::getMin() const { return pimpl->getMin<bool>(); }
template<> VT_EXPORT int ParameterInfo::getMax() const { return pimpl->getMax<int>(); }
template<> VT_EXPORT double ParameterInfo::getMax() const { return pimpl->getMax<double>(); }
template<> VT_EXPORT bool ParameterInfo::getMax() const { return pimpl->getMax<bool>(); }
template<> VT_EXPORT int ParameterInfo::getInc() const { return pimpl->getInc<int>(); }
template<> VT_EXPORT double ParameterInfo::getInc() const { return pimpl->getInc<double>(); }
template<> VT_EXPORT bool ParameterInfo::getInc() const { return pimpl->getInc<bool>(); }
#endif // DOXYGEN_SHOULD_SKIP_THIS

} // namespace

