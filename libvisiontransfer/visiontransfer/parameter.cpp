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

#include <visiontransfer/parameter.h>

namespace visiontransfer {
namespace param {

using namespace internal;

Parameter::Parameter(const std::string& uid)
: uid(uid), name(uid), governorType(GOVERNOR_NONE), invokeGovernorOnInit(false), accessForConfig(ACCESS_NONE), accessForApi(ACCESS_NONE), interactionHint(INTERACTION_ACTIVE), isModified(false) {
}

std::string Parameter::interpolateCommandLine(const ParameterValue& newVal) {
    std::string result = governorString;
    std::set<char> subst{'P', 'O', 'N', 'E', 'D'};
    std::ostringstream ss;
    char what;
    size_t where = -1;
    while (true) {
        int foundAt = -1;
        int foundRightmost = -1;
        what = 0;
        for (auto ch: subst) {
            std::string lookfor = "%";
            lookfor += ch;
            foundAt = result.rfind(lookfor, where);
            if (foundAt > foundRightmost) {
                foundRightmost = foundAt;
                what = ch;
            }
        }
        if (foundRightmost >= 0) {
            ss.str("");
            switch(what) {
                case 'P':
                    ss << "\"" << getUid() << "\"";
                    break;
                case 'O':
                    if (isScalar()) {
                        ss << "\"" << getCurrent<std::string>() << "\"";
                    } else {
                        bool first = true;
                        for (auto e: getTensorData()) {
                            if (first) first = false;
                            else ss << " ";
                            ss << e;
                        }
                    }
                    break;
                case 'E':
                    ss << getTensorDimension();
                    if (isTensor()) {
                        auto sh = getTensorShape();
                        for (auto d: sh) {
                            ss << " " << d;
                        }
                    }
                    break;
                case 'D':
                    ss << newVal.getTensorDimension();
                    if (isTensor()) {
                        auto sh = newVal.getTensorShape();
                        for (auto d: sh) {
                            ss << " " << d;
                        }
                    }
                    break;
                case 'N':
                default:
                    if (newVal.isScalar()) {
                        ss << "\"" << newVal.getValue<std::string>() << "\"";
                    } else {
                        bool first = true;
                        for (auto e: newVal.getTensorData()) {
                            if (first) first = false;
                            else ss << " ";
                            ss << e;
                        }
                    }
                    break;
            }
            result.replace(foundRightmost, 2, ss.str());
        }
        if (where == 0) break; // cannot go further left
        where = (foundRightmost > 0) ? foundRightmost-1 : 0;
    }
    return result;
}

Parameter& Parameter::setCurrentFrom(const Parameter& from) {
    if (isTensor()) {
        if(currentValue.getTensorShape() != from.getTensorShape()) {
            throw std::runtime_error("Cannot assign tensors with unequal shape");
        }
        setTensorData(from.getTensorData());
    } else {
        currentValue.setType(getType());
        // always cache string
        switch (getType()) {
            case ParameterValue::ParameterType::TYPE_INT:
                currentValue.setValue(from.getCurrent<int>()); break;
            case ParameterValue::ParameterType::TYPE_DOUBLE:
                currentValue.setValue(from.getCurrent<double>()); break;
            case ParameterValue::ParameterType::TYPE_STRING:
            case ParameterValue::ParameterType::TYPE_SAFESTRING:
            case ParameterValue::ParameterType::TYPE_COMMAND:
                currentValue.setValue(from.getCurrent<std::string>()); break;
                break;
            case ParameterValue::ParameterType::TYPE_BOOL:
                currentValue.setValue(from.getCurrent<bool>()); break;
                break;
            case ParameterValue::ParameterType::TYPE_TENSOR:
                break; // (handled above)
            case ParameterValue::ParameterType::TYPE_UNDEFINED:
                throw std::runtime_error("Cannot assign a value to an undefined parameter");
        }
        ensureValidCurrent();
    }
    return *this;
}

Parameter& Parameter::setCurrentFromDefault() {
    if (!hasDefault()) {
        throw std::runtime_error(std::string("Cannot set current value from default - no default value set for ") + getUid());
    }
    switch (getType()) {
        case ParameterValue::ParameterType::TYPE_INT:
            currentValue.setType(getType());
            currentValue.setValue(getDefault<int>());
            break;
        case ParameterValue::ParameterType::TYPE_DOUBLE:
            currentValue.setType(getType());
            currentValue.setValue(getDefault<double>());
            break;
        case ParameterValue::ParameterType::TYPE_STRING:
        case ParameterValue::ParameterType::TYPE_SAFESTRING:
            currentValue.setType(getType());
            currentValue.setValue(getDefault<std::string>());
            break;
        case ParameterValue::ParameterType::TYPE_BOOL:
            currentValue.setType(getType());
            currentValue.setValue(getDefault<bool>());
            break;
        case ParameterValue::ParameterType::TYPE_TENSOR:
            if (hasCurrent() && (currentValue.getTensorNumElements() != defaultValue.getTensorNumElements())) {
                throw std::runtime_error(std::string("Mismatching current and default tensor sizes for ") + getUid());
            }
            currentValue.setType(getType());
            currentValue.setTensorData(defaultValue.getTensorData());
            break;
        case ParameterValue::ParameterType::TYPE_COMMAND:
            // Ignore commands for resetting to default value
            break;
        case ParameterValue::ParameterType::TYPE_UNDEFINED:
            throw std::runtime_error("Cannot assign a value to an undefined parameter");
    }
    return *this;
}

template<> VT_EXPORT bool Parameter::enforceIncrement(bool t) {
    return t;
}

template<> VT_EXPORT int Parameter::enforceIncrement(int t) {
    if (hasIncrement() && ((getType()==ParameterValue::TYPE_INT) || (getType()==ParameterValue::TYPE_DOUBLE))) {
        double val = t;
        double inc = getIncrement<double>();
        if (hasRange()) {
            double min = getMin<double>();
            return (int) (min + inc * ((int) (val-min)/inc));
        } else {
            return (int) (inc * ((int) val/inc));
        }
    } else {
        return t;
    }
}

template<> VT_EXPORT double Parameter::enforceIncrement(double t) {
    if (hasIncrement() && ((getType()==ParameterValue::TYPE_INT) || (getType()==ParameterValue::TYPE_DOUBLE))) {
        double val = t;
        double inc = getIncrement<double>();
        if (hasRange()) {
            double min = getMin<double>();
            return min + inc * ((int) (val-min)/inc);
        } else {
            return inc * ((int) val/inc);
        }
    } else {
        return t;
    }
}

template<> VT_EXPORT std::string Parameter::enforceIncrement(std::string t) {
    if (hasIncrement() && ((getType()==ParameterValue::TYPE_INT) || (getType()==ParameterValue::TYPE_DOUBLE))) {
        double val = ConversionHelpers::anyToDouble(t);
        double inc = getIncrement<double>();
        if (hasRange()) {
            double min = getMin<double>();
            return ConversionHelpers::anyToString(min + inc * ((int) (val-min)/inc));
        } else {
            return ConversionHelpers::anyToString(inc * ((int) val/inc));
        }
    } else {
        return t;
    }
}

std::vector<double>& Parameter::getTensorDataReference() {
    if (hasCurrent()) {
        return currentValue.getTensorDataReference();
    } else {
        if (hasDefault()) {
            return defaultValue.getTensorDataReference();
        } else {
            throw std::runtime_error("Tried getTensorDataReference(), but no value set and no default defined");
        }
    }
}

std::vector<double> Parameter::getTensorData() const {
    if (hasCurrent()) {
        return currentValue.getTensorData();
    } else {
        if (hasDefault()) {
            return defaultValue.getTensorData();
        } else {
            throw std::runtime_error("Tried getTensorData(), but no value set and no default defined");
        }
    }
}

std::vector<double>& Parameter::getTensorDefaultDataReference() {
    if (hasDefault()) {
        return defaultValue.getTensorDataReference();
    } else {
        throw std::runtime_error("Tried getTensorDefaultDataReference(), but no value set and no default defined");
    }
}

std::vector<double> Parameter::getTensorDefaultData() const {
    if (hasDefault()) {
        return defaultValue.getTensorData();
    } else {
        throw std::runtime_error("Tried getTensorDefaultData(), but no default defined");
    }
}

bool Parameter::ensureValidDefault() {
    if (!hasDefault()) return false; // Not set yet
    if (isTensor() || isCommand()) return false; // Revision unsupported or not required
    if (hasOptions()) {
        std::string val = defaultValue.getValue<std::string>();
        for (auto& o: validOptions) {
            if (val == o.getValue<std::string>()) {
                return false; // Valid value -> no revision needed
            }
        }
        // Invalid default: select first enum option as a fallback
        defaultValue.setValue<std::string>(validOptions[0].getValue<std::string>());
        return true;
    } else {
        if ((type==ParameterValue::ParameterType::TYPE_INT) || (type==ParameterValue::ParameterType::TYPE_DOUBLE)) {
            if (hasRange()) {
                // Limited range, must check
                double minVal = minValue.getValue<double>();
                double maxVal = maxValue.getValue<double>();
                double val = defaultValue.getValue<double>();
                double incVal = enforceIncrement(val);
                if (val < minVal) {
                    defaultValue.setValue<double>(minVal);
                    return true;
                } else if (val > maxVal) {
                    defaultValue.setValue<double>(maxVal);
                    return true;
                } else if (val != incVal) {
                    // Did not adhere to increment
                    defaultValue.setValue<double>(incVal);
                    return true;
                }
                return false;
            } else {
                // Unlimited range, no revision
                return false;
            }
        } else {
            // No range check for non-numerical parameter
            return false;
        }
    }
}

bool Parameter::ensureValidCurrent() {
    if (!hasCurrent()) return false; // Not set yet
    if (isTensor() || isCommand()) return false; // Revision unsupported or not required
    if (hasOptions()) {
        std::string val = currentValue.getValue<std::string>();
        for (auto& o: validOptions) {
            if (val == o.getValue<std::string>()) {
                return false; // Valid value -> no revision needed
            }
        }
        // Invalid current value: copy default if available, otherwise fallback
        if (hasDefault()) {
            currentValue.setValue<std::string>(defaultValue.getValue<std::string>());
        } else {
            currentValue.setValue<std::string>(validOptions[0].getValue<std::string>());
        }
        return true;
    } else {
        if ((type==ParameterValue::ParameterType::TYPE_INT) || (type==ParameterValue::ParameterType::TYPE_DOUBLE)) {
            if (hasRange()) {
                // Limited range, must check
                double minVal = minValue.getValue<double>();
                double maxVal = maxValue.getValue<double>();
                double val = currentValue.getValue<double>();
                double incVal = enforceIncrement(val);
                if (val < minVal) {
                    currentValue.setValue<double>(minVal);
                    return true;
                } else if (val > maxVal) {
                    currentValue.setValue<double>(maxVal);
                    return true;
                } else if (val != incVal) {
                    // Did not adhere to increment
                    currentValue.setValue<double>(incVal);
                    return true;
                }
                return false;
            } else {
                // Unlimited range, no revision
                return false;
            }
        } else {
            // No range check for non-numerical parameter
            return false;
        }
    }
}

} // namespace param
} // namespace visiontransfer


