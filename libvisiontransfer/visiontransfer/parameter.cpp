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

#include <visiontransfer/parameter.h>

namespace visiontransfer {
namespace param {

using namespace internal;

class Parameter::Pimpl {
public:
    Pimpl();
    Pimpl(const std::string& uid);
    Pimpl(const Pimpl& other);
    static void copyData(Pimpl& dst, const Pimpl& src);
    std::string getUid() const { return uid; }
    std::string getName() const { return name; }
    std::string getModuleName() const { return modulename; }
    std::string getCategoryName() const { return categoryname; }
    std::string getDescription() const { return description; }
    std::string getUnit() const { return unit; }
    inline ParameterValue::ParameterType getType() const { return type; }
    ParameterAccessMode getAccessForConfig() const { return accessForConfig; }
    ParameterAccessMode getAccessForApi() const { return accessForApi; }
    inline Pimpl& setAccessForConfig(ParameterAccessMode mode) { accessForConfig = mode; return *this; }
    inline Pimpl& setAccessForApi(ParameterAccessMode mode) { accessForApi = mode; return *this; }
    ParameterInteractionHint getInteractionHint() const { return interactionHint; }
    inline Pimpl& setInteractionHint(ParameterInteractionHint hint) { interactionHint = hint; return *this; }
    bool getIsModified() const { return isModified; }
    bool getIsPolled() const { return isPolledForUpdates; }
    inline Pimpl& setIsPolled(bool mod) { isPolledForUpdates = isCommand() ? false : mod; return *this; }
    inline Pimpl& setIsModified(bool mod) { isModified = isCommand() ? false : mod; return *this; }
    GovernorType getGovernorType() const { return governorType; }
    std::string getGovernorString() const { return governorString; }
    inline Pimpl& setGovernor(GovernorType govType, const std::string& govStr) { governorType = govType; governorString = govStr; return *this; }
    inline Pimpl& setGovernorPollString(const std::string& govStr) { governorPollString = govStr; return *this; }
    bool getInvokeGovernorOnInit() const { return invokeGovernorOnInit; }
    Pimpl& setInvokeGovernorOnInit(bool invoke) { invokeGovernorOnInit = invoke; return *this; }
    std::string interpolateCommandLine(const ParameterValue& newVal, GovernorFunction fn = GOVERNOR_FN_CHANGE_VALUE);
    bool isTensor() const { return type == ParameterValue::ParameterType::TYPE_TENSOR; }
    bool isScalar() const { return type != ParameterValue::ParameterType::TYPE_TENSOR; }
    bool isCommand() const { return currentValue.isCommand(); }
    unsigned int getTensorDimension() const {
        return currentValue.isDefined() ? currentValue.getTensorDimension() : defaultValue.getTensorDimension(); }
    std::vector<unsigned int> getTensorShape() const {
        return currentValue.isDefined() ? currentValue.getTensorShape() : defaultValue.getTensorShape();}
    unsigned int getTensorNumElements() const {
        return currentValue.isDefined() ? currentValue.getTensorNumElements() : defaultValue.getTensorNumElements();}
    std::vector<double> getTensorData() const;
    std::vector<double> getTensorDefaultData() const;
    std::vector<double>& getTensorDataReference();
    std::vector<double>& getTensorDefaultDataReference();
    inline Pimpl& setTensorData(const std::vector<double>& data) {
        currentValue.setTensorData(data);
        return *this;
    }
    inline Pimpl& setTensorDefaultData(const std::vector<double>& data) {
        defaultValue.setTensorData(data);
        return *this;
    }
    inline Pimpl& setName(const std::string& name) { this->name = name;         return *this; }
    inline Pimpl& setModuleName(const std::string& n) { this->modulename = n; return *this; }
    inline Pimpl& setCategoryName(const std::string& n) { this->categoryname = n; return *this; }
    inline Pimpl& setDescription(const std::string& d) { this->description = d; return *this; }
    inline Pimpl& setUnit(const std::string& d) { this->unit = d; return *this; }
    inline Pimpl& setType(ParameterValue::ParameterType t) {
        this->type = t;
        // min, max, increment, options left undefined here
        if (t==ParameterValue::ParameterType::TYPE_COMMAND) {
            // Commands no not have to have an initialized prior value
            defaultValue.setType(this->type);
            currentValue.setType(this->type);
            defaultValue.setValue("");
            currentValue.setValue("");
        }
        return *this;
    }
    inline Pimpl& setAsTensor(const std::vector<unsigned int>& shape) {
        setType(ParameterValue::TYPE_TENSOR);
        defaultValue.setTensorShape(shape);
        currentValue.setTensorShape(shape);
        return *this;
    }
    template<typename T>
    bool isValidNewValue(T t) const {
        if (validOptions.size()) {
            // enum-style list of options
            for (auto& o: validOptions) {
                if (o.getValue<T>() == t) return true;
            }
            return false;
        } else {
            if ((type==ParameterValue::ParameterType::TYPE_INT) || (type==ParameterValue::ParameterType::TYPE_DOUBLE)) {
                if (minValue.isUndefined() || maxValue.isUndefined()) {
                    // unlimited range numerical
                    return true;
                } else {
                    // limited range numerical
                    double val = internal::ConversionHelpers::anyToDouble(t);
                    return val>=minValue.getValue<double>() && val<=maxValue.getValue<double>();
                }
            } else {
                // non-numerical, cannot range-restrict
                return true;
            }
        }
    }
    bool ensureValidDefault();
    bool ensureValidCurrent();
    template<typename T> T enforceIncrement(T t);
    template<typename T>
    Pimpl& setDefault(T t) {
        defaultValue.setType(getType());
        defaultValue.setValue(enforceIncrement(t));
        ensureValidDefault();
        return *this;
    }
    template<typename T>
    Pimpl& setRange(T mn, T mx) {
        minValue.setType(type);
        maxValue.setType(type);
        minValue.setValue(mn);
        maxValue.setValue(mx);
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    Pimpl& unsetRange() {
        minValue.setType(ParameterValue::ParameterType::TYPE_UNDEFINED);
        maxValue.setType(ParameterValue::ParameterType::TYPE_UNDEFINED);
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    template<typename T>
    Pimpl& setIncrement(T t) {
        incrementValue.setType(type);
        incrementValue.setValue(t);
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    template<typename T>
    Pimpl& setCurrent(T t) {
        currentValue.setType(getType());
        currentValue.setValue(enforceIncrement(t));
        ensureValidCurrent();
        return *this;
    }
    Pimpl& setCurrentFrom(const Pimpl& from);
    Pimpl& setCurrentFromDefault();
    template<typename T>
    Pimpl& setOptions(const std::vector<T>& opts, const std::vector<std::string>& descriptions) {
        if (opts.size() != descriptions.size()) throw std::runtime_error("Option list and description list of mismatched size");
        validOptions.clear();
        validOptionDescriptions.clear();
        for (unsigned int i=0; i<opts.size(); ++i) {
            validOptions.push_back(ParameterValue().setType(type).setValue(opts[i]));
            validOptionDescriptions.push_back(descriptions[i]);
        }
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    template<typename T>
    Pimpl& setOptions(std::initializer_list<T> opts, std::initializer_list<std::string> descriptions) {
        std::vector<T> tmpOpts(opts);
        std::vector<std::string> tmpComm(descriptions);
        return setOptions(tmpOpts, tmpComm);
    }
    template<typename T>
    std::vector<T> getOptions() const {
        std::vector<T> vec;
        for (auto& o: validOptions) {
            vec.push_back(o.getValue<T>());
        }
        return vec;
    }
    std::vector<std::string> getOptionDescriptions() const {
        return validOptionDescriptions;
    }
    inline ParameterValue getCurrentParameterValue() {
        if (hasCurrent()) {
            return currentValue;
        } else {
            if (hasDefault()) {
                return defaultValue;
            } else {
                throw std::runtime_error(std::string("Tried getCurrent(), but no value set and no default defined for ") + getUid());
            }
        }
    }
    template<typename T>
    T getCurrent() const {
        if (hasCurrent()) {
            return currentValue.getValue<T>();
        } else {
            if (hasDefault()) {
                return defaultValue.getValue<T>();
            } else {
                throw std::runtime_error(std::string("Tried getCurrent(), but no value set and no default defined for ") + getUid());
            }
        }
    }
    inline ParameterValue getDefaultParameterValue() {
        return defaultValue;
    }
    template<typename T>
    T getDefault() const {
        return defaultValue.getValue<T>();
    }
    template<typename T>
    T getMin() const {
        return minValue.isDefined() ? (minValue.getValue<T>()) : (std::numeric_limits<T>::lowest());
    }
    template<typename T>
    T getMax() const {
        return maxValue.isDefined() ? (maxValue.getValue<T>()) : ((std::numeric_limits<T>::max)());
    }
    template<typename T>
    T getIncrement() const {
        return incrementValue.isDefined() ? (incrementValue.getValue<T>()) : internal::ConversionHelpers::toStringIfStringExpected<T>(1);
    }
    bool hasOptions() const {
        return validOptions.size() > 0;
    }
    bool hasCurrent() const {
        if (type == ParameterValue::ParameterType::TYPE_TENSOR) {
            // For tensors: the data array must also have been set first
            return currentValue.isDefined() && (currentValue.getTensorCurrentDataSize() == currentValue.getTensorNumElements());
        } else {
            return currentValue.isDefined();
        }
    }
    bool hasDefault() const {
        if (defaultValue.isTensor()) {
            // For tensors: the data array must also have been set first
            return defaultValue.isDefined() && (defaultValue.getTensorCurrentDataSize() == defaultValue.getTensorNumElements());
        } else {
            return defaultValue.isDefined();
        }
    }
    bool hasRange() const {
        return maxValue.isDefined();
    }
    bool hasIncrement() const {
        return incrementValue.isDefined();
    }
    double at(unsigned int x) { return getCurrentParameterValue().tensorElementAt(x); }
    double at(unsigned int y, unsigned int x) { return getCurrentParameterValue().tensorElementAt(y, x); }
    double at(unsigned int z, unsigned int y, unsigned int x) { return getCurrentParameterValue().tensorElementAt(z, y, x); }


private:
    std::string uid;
    std::string name;
    std::string modulename;    // Broad association to a module (e.g. "settings", "cam0", "cam1", etc.)
    std::string categoryname;  // Free-form, module specific, category grouping (e.g. Aravis feature categories)
    std::string description;
    std::string unit;
    ParameterValue::ParameterType type;

    ParameterValue defaultValue;
    ParameterValue currentValue;
    ParameterValue minValue;       // minValue.isUndefined() until range has been set
    ParameterValue maxValue;
    ParameterValue incrementValue; // incrementValue.isUndefined() until increment set
    std::vector<ParameterValue> validOptions;
    std::vector<std::string> validOptionDescriptions;

    // The following fields are used in nvparamd (the master) and not set elsewhere
    GovernorType governorType;
    std::string governorString; // D-Bus address or shell command line, respectively
    std::string governorPollString; //                    '' (for polling)
    // 'oninit' action - ignore, or the same as on a change.
    // This is only used in the parameter daemon, this variable need not be relayed.
    bool invokeGovernorOnInit;

    ParameterAccessMode accessForConfig;
    ParameterAccessMode accessForApi;

    ParameterInteractionHint interactionHint;
    bool isModified;
    bool isPolledForUpdates;
    bool isPollResult;
};

// Pimpl members

// static
void Parameter::Pimpl::copyData(Parameter::Pimpl& dst, const Parameter::Pimpl& src) {
    dst.uid = src.uid;
    dst.name = src.name;
    dst.modulename = src.modulename;
    dst.categoryname = src.categoryname;
    dst.description = src.description;
    dst.unit = src.unit;
    dst.type = src.type;
    dst.defaultValue = src.defaultValue;
    dst.currentValue = src.currentValue;
    dst.minValue = src.minValue;
    dst.maxValue = src.maxValue;
    dst.incrementValue = src.incrementValue;
    dst.validOptions = src.validOptions;
    dst.validOptionDescriptions = src.validOptionDescriptions;
    dst.governorType = src.governorType;
    dst.governorString = src.governorString;
    dst.governorPollString = src.governorPollString;
    dst.invokeGovernorOnInit = src.invokeGovernorOnInit;
    dst.accessForConfig = src.accessForConfig;
    dst.accessForApi = src.accessForApi;
    dst.interactionHint = src.interactionHint;
    dst.isModified = src.isModified;
    dst.isPolledForUpdates = src.isPolledForUpdates;
    dst.isPollResult = src.isPollResult;
}

Parameter::Pimpl::Pimpl()
: uid("undefined"), name("undefined"), governorType(GOVERNOR_NONE), invokeGovernorOnInit(false), accessForConfig(ACCESS_NONE), accessForApi(ACCESS_NONE), interactionHint(INTERACTION_ACTIVE), isModified(false), isPolledForUpdates(false) {
}

Parameter::Pimpl::Pimpl(const std::string& uid)
: uid(uid), name(uid), governorType(GOVERNOR_NONE), invokeGovernorOnInit(false), accessForConfig(ACCESS_NONE), accessForApi(ACCESS_NONE), interactionHint(INTERACTION_ACTIVE), isModified(false), isPolledForUpdates(false) {
}
Parameter::Pimpl::Pimpl(const Parameter::Pimpl& other) {
    copyData(*this, other);
}

std::string Parameter::Pimpl::interpolateCommandLine(const ParameterValue& newVal, GovernorFunction govFn) {
    std::string result = (govFn==GOVERNOR_FN_CHANGE_VALUE) ? governorString : governorPollString;
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
            foundAt = (int) result.rfind(lookfor, where);
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

Parameter::Pimpl& Parameter::Pimpl::setCurrentFrom(const Parameter::Pimpl& from) {
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

Parameter::Pimpl& Parameter::Pimpl::setCurrentFromDefault() {
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

template<> VT_EXPORT bool Parameter::Pimpl::enforceIncrement(bool t) {
    return t;
}

template<> VT_EXPORT int Parameter::Pimpl::enforceIncrement(int t) {
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

template<> VT_EXPORT double Parameter::Pimpl::enforceIncrement(double t) {
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

template<> VT_EXPORT std::string Parameter::Pimpl::enforceIncrement(std::string t) {
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

std::vector<double>& Parameter::Pimpl::getTensorDataReference() {
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

std::vector<double> Parameter::Pimpl::getTensorData() const {
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

std::vector<double>& Parameter::Pimpl::getTensorDefaultDataReference() {
    if (hasDefault()) {
        return defaultValue.getTensorDataReference();
    } else {
        throw std::runtime_error("Tried getTensorDefaultDataReference(), but no value set and no default defined");
    }
}

std::vector<double> Parameter::Pimpl::getTensorDefaultData() const {
    if (hasDefault()) {
        return defaultValue.getTensorData();
    } else {
        throw std::runtime_error("Tried getTensorDefaultData(), but no default defined");
    }
}

bool Parameter::Pimpl::ensureValidDefault() {
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

bool Parameter::Pimpl::ensureValidCurrent() {
    if (!hasCurrent()) return false; // Not set yet
    if (isTensor() || isCommand()) return false; // Revision unsupported or not required
    if (hasOptions()) {
        std::string val = currentValue.getValue<std::string>();
        for (auto& o: validOptions) {
            if (val == o.getValue<std::string>()) {
                return false; // Valid value -> no revision needed
            }
        }
        // Invalid current value: copy default if available and valid
        if (hasDefault()) {
            std::string defVal = defaultValue.getValue<std::string>();
            for (auto& o: validOptions) {
                if (defVal == o.getValue<std::string>()) {
                    currentValue.setValue<std::string>(defVal);
                    return true; // Default value is still valid for current option set
                }
            }
        }
        // The only venue left is to fall back to the first entry
        currentValue.setValue<std::string>(validOptions[0].getValue<std::string>());
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




//
//
// External (API) class
//
//

Parameter::Parameter()
: pimpl(new Parameter::Pimpl()) {
}
Parameter::Parameter(const std::string& uid)
: pimpl(new Parameter::Pimpl(uid)) {
}
Parameter::Parameter(const Parameter& other)
: pimpl(new Parameter::Pimpl()) {
    Parameter::Pimpl::copyData(*pimpl, *(other.pimpl));
}
Parameter::~Parameter() {
    delete pimpl;
}
Parameter& Parameter::operator= (const Parameter& other) {
    Parameter::Pimpl::copyData(*pimpl, *(other.pimpl));
    return *this;
}

std::string Parameter::getUid() const {
    return pimpl->getUid();
}
std::string Parameter::getName() const {
    return pimpl->getName();
}
std::string Parameter::getModuleName() const {
    return pimpl->getModuleName();
}
std::string Parameter::getCategoryName() const {
    return pimpl->getCategoryName();
}
std::string Parameter::getDescription() const {
    return pimpl->getDescription();
}
std::string Parameter::getUnit() const {
    return pimpl->getUnit();
}
ParameterValue::ParameterType Parameter::getType() const {
    return pimpl->getType();
}
Parameter::ParameterAccessMode Parameter::getAccessForConfig() const {
    return pimpl->getAccessForConfig();
}
Parameter::ParameterAccessMode Parameter::getAccessForApi() const {
    return pimpl->getAccessForApi();
}

Parameter& Parameter::setAccessForConfig(Parameter::ParameterAccessMode mode) {
    pimpl->setAccessForConfig(mode);
    return *this;
}
Parameter& Parameter::setAccessForApi(Parameter::ParameterAccessMode mode) {
    pimpl->setAccessForApi(mode);
    return *this;
}
Parameter::ParameterInteractionHint Parameter::getInteractionHint() const {
    return pimpl->getInteractionHint();
}
Parameter& Parameter::setInteractionHint(Parameter::ParameterInteractionHint hint) {
    pimpl->setInteractionHint(hint);
    return *this;
}
bool Parameter::getIsModified() const {
    return pimpl->getIsModified();
}
bool Parameter::getIsPolled() const {
    return pimpl->getIsPolled();
}
Parameter& Parameter::setIsPolled(bool mod) {
    pimpl->setIsPolled(mod);
    return *this;
}
Parameter& Parameter::setIsModified(bool mod) {
    pimpl->setIsModified(mod);
    return *this;
}
Parameter::GovernorType Parameter::getGovernorType() const {
    return pimpl->getGovernorType();
}
std::string Parameter::getGovernorString() const {
    return pimpl->getGovernorString();
}
Parameter& Parameter::setGovernor(Parameter::GovernorType govType, const std::string& govStr) {
    pimpl->setGovernor(govType, govStr);
    return *this;
}
Parameter& Parameter::setGovernorPollString(const std::string& govStr) {
    pimpl->setGovernorPollString(govStr);
    return *this;
}
bool Parameter::getInvokeGovernorOnInit() const {
    return pimpl->getInvokeGovernorOnInit();
}
Parameter& Parameter::setInvokeGovernorOnInit(bool invoke) {
    pimpl->setInvokeGovernorOnInit(invoke);
    return *this;
}
std::string Parameter::interpolateCommandLine(const ParameterValue& newVal, Parameter::GovernorFunction fn) {
    return pimpl->interpolateCommandLine(newVal, fn);
}
bool Parameter::isTensor() const {
    return pimpl->isTensor();
}
bool Parameter::isScalar() const {
    return pimpl->isScalar();
}
bool Parameter::isCommand() const {
    return pimpl->isCommand();
}
unsigned int Parameter::getTensorDimension() const {
    return pimpl->getTensorDimension();
}
std::vector<unsigned int> Parameter::getTensorShape() const {
    return pimpl->getTensorShape();
}
unsigned int Parameter::getTensorNumElements() const {
    return pimpl->getTensorNumElements();
}
std::vector<double> Parameter::getTensorData() const {
    return pimpl->getTensorData();
}
std::vector<double> Parameter::getTensorDefaultData() const {
    return pimpl->getTensorDefaultData();
}
std::vector<double>& Parameter::getTensorDataReference() {
    return pimpl->getTensorDataReference();
}
std::vector<double>& Parameter::getTensorDefaultDataReference() {
    return pimpl->getTensorDefaultDataReference();
}
Parameter& Parameter::setTensorData(const std::vector<double>& data) {
    pimpl->setTensorData(data);
    return *this;
}
Parameter& Parameter::setTensorDefaultData(const std::vector<double>& data) {
    pimpl->setTensorDefaultData(data);
    return *this;
}

Parameter& Parameter::setName(const std::string& name) {
    pimpl->setName(name);
    return *this;
}
Parameter& Parameter::setModuleName(const std::string& n) {
    pimpl->setModuleName(n);
    return *this;
}
Parameter& Parameter::setCategoryName(const std::string& n) {
    pimpl->setCategoryName(n);
    return *this;
}
Parameter& Parameter::setDescription(const std::string& d) {
    pimpl->setDescription(d);
    return *this;
}
Parameter& Parameter::setUnit(const std::string& d) {
    pimpl->setUnit(d);
    return *this;
}
Parameter& Parameter::setType(ParameterValue::ParameterType t) {
    pimpl->setType(t);
    return *this;
}
Parameter& Parameter::setAsTensor(const std::vector<unsigned int>& shape) {
    pimpl->setAsTensor(shape);
    return *this;
}
bool Parameter::ensureValidDefault() {
    return pimpl->ensureValidDefault();
}
bool Parameter::ensureValidCurrent() {
    return pimpl->ensureValidCurrent();
}

// Specializations of isValidNewValue
template<> VT_EXPORT bool Parameter::isValidNewValue(int t) const { return pimpl->isValidNewValue<int>(t); }
template<> VT_EXPORT bool Parameter::isValidNewValue(bool t) const { return pimpl->isValidNewValue<bool>(t); }
template<> VT_EXPORT bool Parameter::isValidNewValue(double t) const { return pimpl->isValidNewValue<double>(t); }
template<> VT_EXPORT bool Parameter::isValidNewValue(std::string t) const { return pimpl->isValidNewValue<std::string>(t); }

// Specializations of enforceIncrement
template<> VT_EXPORT int Parameter::enforceIncrement(int t) { return pimpl->enforceIncrement<int>(t); }
template<> VT_EXPORT bool Parameter::enforceIncrement(bool t) { return pimpl->enforceIncrement<bool>(t); }
template<> VT_EXPORT double Parameter::enforceIncrement(double t) { return pimpl->enforceIncrement<double>(t); }
template<> VT_EXPORT std::string Parameter::enforceIncrement(std::string t) { return pimpl->enforceIncrement<std::string>(t); }

// Specializations of setDefault
template<> VT_EXPORT Parameter& Parameter::setDefault(int t) { pimpl->setDefault<int>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setDefault(bool t) { pimpl->setDefault<bool>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setDefault(double t) { pimpl->setDefault<double>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setDefault(std::string t) { pimpl->setDefault<std::string>(t); return *this; }

// Specializations of setRange
template<> VT_EXPORT Parameter& Parameter::setRange(int mn, int mx) { pimpl->setRange<int>(mn, mx); return *this; }
template<> VT_EXPORT Parameter& Parameter::setRange(bool mn, bool mx) { pimpl->setRange<bool>(mn, mx); return *this; }
template<> VT_EXPORT Parameter& Parameter::setRange(double mn, double mx) { pimpl->setRange<double>(mn, mx); return *this; }
template<> VT_EXPORT Parameter& Parameter::setRange(std::string mn, std::string mx) { pimpl->setRange<std::string>(mn, mx); return *this; }

Parameter& Parameter::unsetRange() {
    pimpl->unsetRange();
    return *this;
}

// Specializations of setIncrement
template<> VT_EXPORT Parameter& Parameter::setIncrement(int t) { pimpl->setIncrement<int>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setIncrement(bool t) { pimpl->setIncrement<bool>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setIncrement(double t) { pimpl->setIncrement<double>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setIncrement(std::string t) { pimpl->setIncrement<std::string>(t); return *this; }

// Specializations of setCurrent
template<> VT_EXPORT Parameter& Parameter::setCurrent(int t) { pimpl->setCurrent<int>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setCurrent(bool t) { pimpl->setCurrent<bool>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setCurrent(double t) { pimpl->setCurrent<double>(t); return *this; }
template<> VT_EXPORT Parameter& Parameter::setCurrent(std::string t) { pimpl->setCurrent<std::string>(t); return *this; }

Parameter& Parameter::setCurrentFrom(const Parameter& from) {
    pimpl->setCurrentFrom(*(from.pimpl));
    return *this;
}
Parameter& Parameter::setCurrentFromDefault() {
    pimpl->setCurrentFromDefault();
    return *this;
}

// Specializations of setOptions
template<> VT_EXPORT Parameter& Parameter::setOptions(const std::vector<int>& opts, const std::vector<std::string>& descriptions) { pimpl->setOptions<int>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(const std::vector<bool>& opts, const std::vector<std::string>& descriptions) { pimpl->setOptions<bool>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(const std::vector<double>& opts, const std::vector<std::string>& descriptions) { pimpl->setOptions<double>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(const std::vector<std::string>& opts, const std::vector<std::string>& descriptions) { pimpl->setOptions<std::string>(opts, descriptions); return *this; }

// Specializations of setOptions (initializer list)
template<> VT_EXPORT Parameter& Parameter::setOptions(std::initializer_list<int> opts, std::initializer_list<std::string> descriptions) { pimpl->setOptions<int>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(std::initializer_list<bool> opts, std::initializer_list<std::string> descriptions) { pimpl->setOptions<bool>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(std::initializer_list<double> opts, std::initializer_list<std::string> descriptions) { pimpl->setOptions<double>(opts, descriptions); return *this; }
template<> VT_EXPORT Parameter& Parameter::setOptions(std::initializer_list<std::string> opts, std::initializer_list<std::string> descriptions) { pimpl->setOptions<std::string>(opts, descriptions); return *this; }

// Specializations of getOptions
template<> VT_EXPORT std::vector<int> Parameter::getOptions() const { return pimpl->getOptions<int>(); }
template<> VT_EXPORT std::vector<bool> Parameter::getOptions() const { return pimpl->getOptions<bool>(); }
template<> VT_EXPORT std::vector<double> Parameter::getOptions() const { return pimpl->getOptions<double>(); }
template<> VT_EXPORT std::vector<std::string> Parameter::getOptions() const { return pimpl->getOptions<std::string>(); }

std::vector<std::string> Parameter::getOptionDescriptions() const {
    return pimpl->getOptionDescriptions();
}

ParameterValue Parameter::getCurrentParameterValue() {
    return pimpl->getCurrentParameterValue();
}
ParameterValue Parameter::getDefaultParameterValue() {
    return pimpl->getDefaultParameterValue();
}

// Specializations of getCurrent
template<> VT_EXPORT int Parameter::getCurrent() const { return pimpl->getCurrent<int>(); }
template<> VT_EXPORT bool Parameter::getCurrent() const { return pimpl->getCurrent<bool>(); }
template<> VT_EXPORT double Parameter::getCurrent() const { return pimpl->getCurrent<double>(); }
template<> VT_EXPORT std::string Parameter::getCurrent() const { return pimpl->getCurrent<std::string>(); }
// Specializations of getDefault
template<> VT_EXPORT int Parameter::getDefault() const { return pimpl->getDefault<int>(); }
template<> VT_EXPORT bool Parameter::getDefault() const { return pimpl->getDefault<bool>(); }
template<> VT_EXPORT double Parameter::getDefault() const { return pimpl->getDefault<double>(); }
template<> VT_EXPORT std::string Parameter::getDefault() const { return pimpl->getDefault<std::string>(); }
// Specializations of getMin
template<> VT_EXPORT int Parameter::getMin() const { return pimpl->getMin<int>(); }
template<> VT_EXPORT bool Parameter::getMin() const { return pimpl->getMin<bool>(); }
template<> VT_EXPORT double Parameter::getMin() const { return pimpl->getMin<double>(); }
template<> VT_EXPORT std::string Parameter::getMin() const { return pimpl->getMin<std::string>(); }
// Specializations of getMax
template<> VT_EXPORT int Parameter::getMax() const { return pimpl->getMax<int>(); }
template<> VT_EXPORT bool Parameter::getMax() const { return pimpl->getMax<bool>(); }
template<> VT_EXPORT double Parameter::getMax() const { return pimpl->getMax<double>(); }
template<> VT_EXPORT std::string Parameter::getMax() const { return pimpl->getMax<std::string>(); }
// Specializations of getIncrement
template<> VT_EXPORT int Parameter::getIncrement() const { return pimpl->getIncrement<int>(); }
template<> VT_EXPORT bool Parameter::getIncrement() const { return pimpl->getIncrement<bool>(); }
template<> VT_EXPORT double Parameter::getIncrement() const { return pimpl->getIncrement<double>(); }
template<> VT_EXPORT std::string Parameter::getIncrement() const { return pimpl->getIncrement<std::string>(); }

bool Parameter::hasOptions() const {
    return pimpl->hasOptions();
}
bool Parameter::hasCurrent() const {
    return pimpl->hasCurrent();
}
bool Parameter::hasDefault() const {
    return pimpl->hasDefault();
}
bool Parameter::hasRange() const {
    return pimpl->hasRange();
}
bool Parameter::hasIncrement() const {
    return pimpl->hasIncrement();
}

double Parameter::at(unsigned int x) {
    return pimpl->at(x);
}
double Parameter::at(unsigned int y, unsigned int x) {
    return pimpl->at(y, x);
}
double Parameter::at(unsigned int z, unsigned int y, unsigned int x) {
    return pimpl->at(z, y, x);
}


} // namespace param
} // namespace visiontransfer


