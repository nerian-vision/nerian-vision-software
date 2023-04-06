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

#ifndef VISIONTRANSFER_PARAMETER_H
#define VISIONTRANSFER_PARAMETER_H

#include <string>
#include <vector>
#include <map>

#include <cstring>
#include <sstream>
#include <iomanip>
#include <limits>
#include <memory>

#include <visiontransfer/common.h>
#include <visiontransfer/parametervalue.h>
#include <visiontransfer/conversionhelpers.h>

namespace visiontransfer {
namespace param {

/**
 * The main Parameter class. A variant-like type with current and
 * default values and a lot of metadata.
 * Parameter data is softly typed, the getters and setters accept
 * a template parameter and are interoperable; with the notable
 * exception of Tensor-type parameters, which have specific accessors.
 *
 * If OpenCv is included ahead of including this file in your project,
 * header-only TYPE_TENSOR<->OpenCv adapters for cv::Size and cv::Mat
 * are made available.
 *
 */
class VT_EXPORT Parameter {
public:
    enum GovernorType {
        GOVERNOR_NONE      = 0,
        GOVERNOR_SHELL     = 1,
        GOVERNOR_DBUS      = 2
    };

    enum ParameterAccessMode {
        ACCESS_NONE        = 0,
        ACCESS_READONLY    = 1,
        ACCESS_READWRITE   = 2
    };

    enum ParameterInteractionHint {
        INTERACTION_INVISIBLE = -1,
        INTERACTION_INACTIVE = 0,
        INTERACTION_ACTIVE = 1
    };

    /** Generate a new Parameter for manual filling */
    Parameter(): uid("undefined"), name("undefined"), governorType(GOVERNOR_NONE), invokeGovernorOnInit(false), accessForConfig(ACCESS_NONE), accessForApi(ACCESS_NONE), interactionHint(INTERACTION_ACTIVE), isModified(false) {}
    /** Generate a new Parameter with specified UID (preferred) for manual filling */
    Parameter(const std::string& uid);
    /** Return the current UID */
    std::string getUid() const { return uid; }
    /** Return the current human-readable name */
    std::string getName() const { return name; }
    /** Return the overarching module name (abstract categorization for GUI etc.) */
    std::string getModuleName() const { return modulename; }
    /** Return the finer-grained category name within a module */
    std::string getCategoryName() const { return categoryname; }
    /** Return the parameter description (explanatory comment string) */
    std::string getDescription() const { return description; }
    /** Return the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    std::string getUnit() const { return unit; }
    /** Return the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    inline ParameterValue::ParameterType getType() const { return type; }
    /** Return the visibility / access mode for the configuration web interface */
    ParameterAccessMode getAccessForConfig() const { return accessForConfig; }
    /** Return the visibility / access mode for the external API bridge */
    ParameterAccessMode getAccessForApi() const { return accessForApi; }
    /** Sets the visibility / access mode for the configuration web interface */
    inline Parameter& setAccessForConfig(ParameterAccessMode mode) { accessForConfig = mode; return *this; }
    /** Sets the visibility / access mode for the external API bridge */
    inline Parameter& setAccessForApi(ParameterAccessMode mode) { accessForApi = mode; return *this; }
    /** Gets the interaction UI hint (invisible / inactive / active) */
    ParameterInteractionHint getInteractionHint() const { return interactionHint; }
    /** Sets the interaction UI hint (invisible / inactive / active) */
    inline Parameter& setInteractionHint(ParameterInteractionHint hint) { interactionHint = hint; return *this; }
    /** Returns whether the Parameter has unsaved (and unreverted) run-time modifications to its previously saved value, according to the parameter server. */
    bool getIsModified() const { return isModified; }
    /** Sets the runtime-modified flag. This is controlled by the device; changing it has no effect in client-side code. */
    inline Parameter& setIsModified(bool mod) { isModified = isCommand() ? false : mod; return *this; }
    /** Return the governor type (whether setting the parameter is controlled by a script, D-Bus, or nothing yet) */
    GovernorType getGovernorType() const { return governorType; }
    /** Return the governor string, i.e. the specific script cmdline or the controlling D-Bus name */
    std::string getGovernorString() const { return governorString; }
    /** Sets the parameter governor, a script or D-Bus name that controls setting the parameter */
    inline Parameter& setGovernor(GovernorType govType, const std::string& govStr) { governorType = govType; governorString = govStr; return *this; }
    /** Gets the oninit action (after first load). False = do nothing; true = perform the same action as on change. Used for calling governor scripts immediately when the parameter daemon initializes. */
    bool getInvokeGovernorOnInit() const { return invokeGovernorOnInit; }
    /** Sets the oninit action (see getInvokeGovernorOnInit). */
    Parameter& setInvokeGovernorOnInit(bool invoke) { invokeGovernorOnInit = invoke; return *this; }
    /** Perform a substition of %-initiated placeholders with correctly
        quoted parameter [meta-]data (for compiling shell commandlines).
    */
    std::string interpolateCommandLine(const ParameterValue& newVal);
    /** Returns true iff the value type of the Parameter is TENSOR */
    bool isTensor() const { return type == ParameterValue::ParameterType::TYPE_TENSOR; }
    /** Returns true iff the value type of the Parameter is scalar, i.e. neither TENSOR nor COMMAND */
    bool isScalar() const { return type != ParameterValue::ParameterType::TYPE_TENSOR; }
    /** Returns true iff the value type of the Parameter is COMMAND (a special string type that corresponds to single-shot actions such as buttons) */
    bool isCommand() const { return currentValue.isCommand(); }
    /** Returns the tensor dimension, i.e. 1 for vectors, 2 for matrices etc. */
    unsigned int getTensorDimension() const {
        return currentValue.isDefined() ? currentValue.getTensorDimension() : defaultValue.getTensorDimension(); }
    /** Returns the tensor shape in all dimensions. This is {rows, columns} for matrices. */
    std::vector<unsigned int> getTensorShape() const {
        return currentValue.isDefined() ? currentValue.getTensorShape() : defaultValue.getTensorShape();}
    /** Returns the total number of elements in the Tensor (product of the sizes in all dimensions). */
    unsigned int getTensorNumElements() const {
        return currentValue.isDefined() ? currentValue.getTensorNumElements() : defaultValue.getTensorNumElements();}
    /** Returns a flat copy of the tensor data as a vector<double> */
    std::vector<double> getTensorData() const;
    /** Returns a flat copy of the tensor default data as a vector<double> (mostly internal use) */
    std::vector<double> getTensorDefaultData() const;
    /** Returns a reference to the tensor data (CAUTION) */
    std::vector<double>& getTensorDataReference();
    /** Returns a reference to the tensor default data (CAUTION) */
    std::vector<double>& getTensorDefaultDataReference();
    /** Sets the tensor data from a flat representation */
    inline Parameter& setTensorData(const std::vector<double>& data) {
        currentValue.setTensorData(data);
        return *this;
    }
    /** Sets the fallback tensor data */
    inline Parameter& setTensorDefaultData(const std::vector<double>& data) {
        defaultValue.setTensorData(data);
        return *this;
    }
#ifdef CV_MAJOR_VERSION
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Size object, yielding a tensor of shape {2} */
    inline Parameter& setTensorFromCvSize(const cv::Size& cvSize) {
        if (currentValue.isDefined() && !(currentValue.isTensor())) {
            throw std::runtime_error("Parameter::setTensorFromCvSize(): refused to overwrite existing non-tensor type");
        }
        if (isTensor() && (getTensorNumElements()!=0)) {
            // Already a tensor; only allow replacement with Size if prior size was 2
            if (getTensorNumElements() != 2) throw std::runtime_error("Parameter::setTensorFromSize(): refused to overwrite tensor with size != 2");
        } else {
            // Newly defined as a Tensor, set as two-element vector
            setAsTensor({2});
        }
        std::vector<double> data = { (double) cvSize.width, (double) cvSize.height };
        currentValue.setTensorData(data);
        defaultValue.setTensorData(data);
        return *this;
    }
    /** Sets an OpenCV Size object from a Tensor-type Parameter, which must be of shape {2}. */
    inline void setCvSizeFromTensor(cv::Size& cvSize) {
        if (getTensorNumElements() != 2) throw std::runtime_error("Parameter::setCvSizeFromTensor(): refused to export Tensor of size!=2 to cv::Size");
        auto val = getCurrentParameterValue();
        cvSize = cv::Size((int) val.tensorElementAt(0), (int) val.tensorElementAt(1));
    }
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Mat object, yielding a two-dimensional tensor of identical shape */
    template<typename T>
    inline Parameter& setTensorFromCvMat(const cv::Mat_<T>& cvMat) {
        if (currentValue.isDefined() && !(currentValue.isTensor())) {
            throw std::runtime_error("Parameter::setTensorFromCvMat(): refused to overwrite existing non-tensor type");
        }
        std::vector<unsigned int> dims = { (unsigned int) cvMat.rows, (unsigned int) cvMat.cols };
        if (isTensor() && (getTensorNumElements()!=0)) {
            // Already a tensor; only allow replacement with Mat data of matching size
            if (getTensorNumElements() != dims[0]*dims[1]) throw std::runtime_error("Parameter::setTensorFromCvMat(): refused to overwrite tensor with cv::Mat of mismatching total size");
        } else {
            // Newly defined as a Tensor, copy the Cv shape
            setAsTensor(dims);
        }
        // Not the fastest way, but less hassle than coping with array casts
        std::vector<double> data;
        for (unsigned int r=0; r<(unsigned int) cvMat.rows; ++r)
            for (unsigned int c=0; c<(unsigned int) cvMat.cols; ++c) {
                data.push_back((double) cvMat(r, c));
            }
        currentValue.setTensorData(data);
        defaultValue.setTensorData(data);
        return *this;
    }
    /** Sets an OpenCV Mat object of arbitrary base type from a Tensor-type Parameter, which must be two-dimensional. */
    template<typename T>
    inline void setCvMatFromTensor(cv::Mat_<T>& cvMat) {
        if (getTensorDimension() != 2) {
            std::ostringstream ss;
            ss << "{";
            for (unsigned int i=0; i<getTensorDimension(); ++i) {
                ss << getTensorShape()[i] << ", ";
            }
            ss << "}";
            ss << " " << getUid() << " " << ((int)getType());
            throw std::runtime_error(std::string("Parameter::setCvMatFromTensor(): refused to export non-2D Tensor to cv::Mat, offending shape is: ")+ss.str());
        }
        auto& refData = getTensorDataReference();
        cv::Mat_<T>(getTensorShape()[0], getTensorShape()[1], (T*)&refData[0]).copyTo(cvMat);
    }
#endif // CV_MAJOR_VERSION
    /** Sets the human-readable name */
    inline Parameter& setName(const std::string& name) { this->name = name;         return *this; }
    /** Sets the overarching module name (abstract categorization for GUI etc.) */
    inline Parameter& setModuleName(const std::string& n) { this->modulename = n; return *this; }
    /** Sets the finer-grained category name within a module */
    inline Parameter& setCategoryName(const std::string& n) { this->categoryname = n; return *this; }
    /** Sets the parameter description (explanatory comment string) */
    inline Parameter& setDescription(const std::string& d) { this->description = d; return *this; }
    /** Sets the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    inline Parameter& setUnit(const std::string& d) { this->unit = d; return *this; }
    /** Sets the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    inline Parameter& setType(ParameterValue::ParameterType t) {
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
    /** Sets a Parameter to be a tensor, specifying the shape at the same time */
    inline Parameter& setAsTensor(const std::vector<unsigned int>& shape) {
        setType(ParameterValue::TYPE_TENSOR);
        defaultValue.setTensorShape(shape);
        currentValue.setTensorShape(shape);
        return *this;
    }
    /** Returns whether the specified value is acceptable for the current Parameter (if it is constrained by range or enum list) */
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
    /** Ensures that the default value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    bool ensureValidDefault();
    /** Ensures that the current value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    bool ensureValidCurrent();
    /** Returns value with any increment applied (starting at minimum, if specified)*/
    template<typename T> T enforceIncrement(T t);
    /** Sets the default value for the Parameter (Caveat: for scalars only) */
    template<typename T>
    Parameter& setDefault(T t) {
        defaultValue.setType(getType());
        defaultValue.setValue(enforceIncrement(t));
        ensureValidDefault();
        return *this;
    }
    /** Sets the valid value range for the Parameter (only checked for numerical scalars) */
    template<typename T>
    Parameter& setRange(T mn, T mx) {
        minValue.setType(type);
        maxValue.setType(type);
        minValue.setValue(mn);
        maxValue.setValue(mx);
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    /** Sets the uniform increment (granularity) for the Parameter (only checked for numerical scalars) */
    template<typename T>
    Parameter& setIncrement(T t) {
        incrementValue.setType(type);
        incrementValue.setValue(t);
        ensureValidDefault();
        ensureValidCurrent();
        return *this;
    }
    /** Sets or overwrites the current value of the Parameter. Type compatibility is enforced and std::runtime_error is thrown for violations. */
    template<typename T>
    Parameter& setCurrent(T t) {
        currentValue.setType(getType());
        currentValue.setValue(enforceIncrement(t));
        ensureValidCurrent();
        return *this;
    }
    /** Copies over the current value from the specified Parameter into the present one */
    Parameter& setCurrentFrom(const Parameter& from);
    /** Copies over the current value from the default value, if specified */
    Parameter& setCurrentFromDefault();
    /** Specifies a vector of possible values and a vector of description strings, making the present Parameter an enum-style option list */
    template<typename T>
    Parameter& setOptions(const std::vector<T>& opts, const std::vector<std::string>& descriptions) {
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
    /** Specifies an initializer_list of possible values and an initializer_list of description strings, making the present Parameter an enum-style option list */
    template<typename T>
    Parameter& setOptions(std::initializer_list<T> opts, std::initializer_list<std::string> descriptions) {
        std::vector<T> tmpOpts(opts);
        std::vector<std::string> tmpComm(descriptions);
        return setOptions(tmpOpts, tmpComm);
    }
    /** Returns the list of possible values for an enum-style Parameter (or an empty list for non-enums). */
    template<typename T>
    std::vector<T> getOptions() const {
        std::vector<T> vec;
        for (auto& o: validOptions) {
            vec.push_back(o.getValue<T>());
        }
        return vec;
    }
    /** Returns the list of option descriptions for an enum-style Parameter (or an empty list for non-enums). */
    std::vector<std::string> getOptionDescriptions() const {
        return validOptionDescriptions;
    }
    /** Returns the ParameterValue representing the Parameters current or default value */
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
    /** Returns the current value for the Parameter. Not supported for tensors (use getTensorData() instead). */
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
    /** Returns the default value for the Parameter. Not supported for tensors. */
    template<typename T>
    T getDefault() const {
        return defaultValue.getValue<T>();
    }
    /** Returns the minimum value for the Parameter (or the lowest representable value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getMin() const {
        return minValue.isDefined() ? (minValue.getValue<T>()) : (std::numeric_limits<T>::lowest());
    }
    /** Returns the maximum value for the Parameter (or the highest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getMax() const {
        return maxValue.isDefined() ? (maxValue.getValue<T>()) : ((std::numeric_limits<T>::max)());
    }
    /** Returns the increment (granularity) for the Parameter (or the smallest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getIncrement() const {
        return incrementValue.isDefined() ? (incrementValue.getValue<T>()) : internal::ConversionHelpers::toStringIfStringExpected<T>(1);
    }
    /** Returns true for an enum-style Parameter (i.e. a list of valid options has been set), false otherwise. */
    bool hasOptions() const {
        return validOptions.size() > 0;
    }
    /** Returns true iff the Parameter has a defined current value */
    bool hasCurrent() const {
        if (type == ParameterValue::ParameterType::TYPE_TENSOR) {
            // For tensors: the data array must also have been set first
            return currentValue.isDefined() && (currentValue.getTensorCurrentDataSize() == currentValue.getTensorNumElements());
        } else {
            return currentValue.isDefined();
        }
    }
    /** Returns true iff the Parameter has a defined default value */
    bool hasDefault() const {
        if (defaultValue.isTensor()) {
            // For tensors: the data array must also have been set first
            return defaultValue.isDefined() && (defaultValue.getTensorCurrentDataSize() == defaultValue.getTensorNumElements());
        } else {
            return defaultValue.isDefined();
        }
    }
    /** Returns true iff the Parameter has a limited range set, false otherwise */
    bool hasRange() const {
        return maxValue.isDefined();
    }
    /** Returns true iff the Parameter has an increment (granularity) set, false otherwise */
    bool hasIncrement() const {
        return incrementValue.isDefined();
    }
    /** Returns the x-th element of a 1-d tensor (vector). Also extends to higher dimensional tensors by indexing the flat data (as if getTensorData() has been used) */
    double at(unsigned int x) { return getCurrentParameterValue().tensorElementAt(x); }
    /** Returns the y-th row, x-th column data element for a 2-dimensional tensor. Fails for other shapes or types */
    double at(unsigned int y, unsigned int x) { return getCurrentParameterValue().tensorElementAt(y, x); }
    /** Returns the z-th slice, y-th row, x-th column data element for a 3-dimensional tensor. Fails for other shapes or types */
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
    // 'oninit' action - ignore, or the same as on a change.
    // This is only used in the parameter daemon, this variable need not be relayed.
    bool invokeGovernorOnInit;

    ParameterAccessMode accessForConfig;
    ParameterAccessMode accessForApi;

    ParameterInteractionHint interactionHint;
    bool isModified;
};

} // namespace param
} // namespace visiontransfer

#endif


