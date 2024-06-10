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
#include <visiontransfer/internal/conversionhelpers.h>

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
class Parameter {

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

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

    enum GovernorFunction {
        GOVERNOR_FN_CHANGE_VALUE = 0,
        GOVERNOR_FN_POLL = 1
    };

    /** Generate a new Parameter for manual filling */
    VT_EXPORT Parameter();
    /** Generate a new Parameter with specified UID (preferred) for manual filling */
    VT_EXPORT Parameter(const std::string& uid);
    /** Generate a copy of a Parameter object */
    VT_EXPORT Parameter(const Parameter& other);
    /** Destructor */
    VT_EXPORT ~Parameter();
    /** Assignment copy operator */
    VT_EXPORT Parameter& operator= (const Parameter& other);
    /** Return the current UID */
    VT_EXPORT std::string getUid() const;
    /** Return the current human-readable name */
    VT_EXPORT std::string getName() const;
    /** Return the overarching module name (abstract categorization for GUI etc.) */
    VT_EXPORT std::string getModuleName() const;
    /** Return the finer-grained category name within a module */
    VT_EXPORT std::string getCategoryName() const;
    /** Return the parameter description (explanatory comment string) */
    VT_EXPORT std::string getDescription() const;
    /** Return the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    VT_EXPORT std::string getUnit() const;
    /** Return the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    VT_EXPORT ParameterValue::ParameterType getType() const;
    /** Return the visibility / access mode for the configuration web interface */
    VT_EXPORT ParameterAccessMode getAccessForConfig() const;
    /** Return the visibility / access mode for the external API bridge */
    VT_EXPORT ParameterAccessMode getAccessForApi() const;
    /** Sets the visibility / access mode for the configuration web interface */
    VT_EXPORT Parameter& setAccessForConfig(ParameterAccessMode mode);
    /** Sets the visibility / access mode for the external API bridge */
    VT_EXPORT Parameter& setAccessForApi(ParameterAccessMode mode);
    /** Gets the interaction UI hint (invisible / inactive / active) */
    VT_EXPORT ParameterInteractionHint getInteractionHint() const;
    /** Sets the interaction UI hint (invisible / inactive / active) */
    VT_EXPORT Parameter& setInteractionHint(ParameterInteractionHint hint);
    /** Returns whether the Parameter has unsaved (and unreverted) run-time modifications to its previously saved value, according to the parameter server. */
    VT_EXPORT bool getIsModified() const;
    /** Returns true iff the tensor is only updated when polled. Such parameters are not synchronized in the background and must be polled (in the backend outside this class) to obtain their up-to-date values. */
    VT_EXPORT bool getIsPolled() const;
    /** Sets the polled-updates-only flag. This is controlled by the device; changing it has no effect in client-side code. */
    VT_EXPORT Parameter& setIsPolled(bool mod);
    /** Sets the runtime-modified flag. This is controlled by the device; changing it has no effect in client-side code. */
    VT_EXPORT Parameter& setIsModified(bool mod);
    /** Return the governor type (whether setting the parameter is controlled by a script, D-Bus, or nothing yet) */
    VT_EXPORT GovernorType getGovernorType() const;
    /** Return the governor string, i.e. the specific script cmdline or the controlling D-Bus name */
    VT_EXPORT std::string getGovernorString() const;
    /** Sets the parameter governor, a script or D-Bus name that controls setting the parameter */
    VT_EXPORT Parameter& setGovernor(GovernorType govType, const std::string& govStr);
    /** Sets the governor, a script or D-Bus name that controls setting the parameter */
    VT_EXPORT Parameter& setGovernorPollString(const std::string& govStr);
    /** Gets the oninit action (after first load). False = do nothing; true = perform the same action as on change. Used for calling governor scripts immediately when the parameter daemon initializes. */
    VT_EXPORT bool getInvokeGovernorOnInit() const;
    /** Sets the oninit action (see getInvokeGovernorOnInit). */
    VT_EXPORT Parameter& setInvokeGovernorOnInit(bool invoke);
    /** Perform a substition of %-initiated placeholders with correctly
        quoted parameter [meta-]data (for compiling shell commandlines).
    */
    VT_EXPORT std::string interpolateCommandLine(const ParameterValue& newVal, GovernorFunction fn = GOVERNOR_FN_CHANGE_VALUE);
    /** Returns true iff the value type of the Parameter is TENSOR */
    VT_EXPORT bool isTensor() const;
    /** Returns true iff the value type of the Parameter is scalar, i.e. neither TENSOR nor COMMAND */
    VT_EXPORT bool isScalar() const;
    /** Returns true iff the value type of the Parameter is COMMAND (a special string type that corresponds to single-shot actions such as buttons) */
    VT_EXPORT bool isCommand() const;
    /** Returns the tensor dimension, i.e. 1 for vectors, 2 for matrices etc. */
    VT_EXPORT unsigned int getTensorDimension() const;
    /** Returns the tensor shape in all dimensions. This is {rows, columns} for matrices. */
    VT_EXPORT std::vector<unsigned int> getTensorShape() const;
    /** Returns the total number of elements in the Tensor (product of the sizes in all dimensions). */
    VT_EXPORT unsigned int getTensorNumElements() const;
    /** Returns a flat copy of the tensor data as a vector<double> */
    VT_EXPORT std::vector<double> getTensorData() const;
    /** Returns a flat copy of the tensor default data as a vector<double> (mostly internal use) */
    VT_EXPORT std::vector<double> getTensorDefaultData() const;
    /** Returns a reference to the tensor data (CAUTION) */
    VT_EXPORT std::vector<double>& getTensorDataReference();
    /** Returns a reference to the tensor default data (CAUTION) */
    VT_EXPORT std::vector<double>& getTensorDefaultDataReference();
    /** Sets the tensor data from a flat representation */
    VT_EXPORT Parameter& setTensorData(const std::vector<double>& data);
    /** Sets the fallback tensor data */
    VT_EXPORT Parameter& setTensorDefaultData(const std::vector<double>& data);
    /** Sets the human-readable name */
    VT_EXPORT Parameter& setName(const std::string& name);
    /** Sets the overarching module name (abstract categorization for GUI etc.) */
    VT_EXPORT Parameter& setModuleName(const std::string& n);
    /** Sets the finer-grained category name within a module */
    VT_EXPORT Parameter& setCategoryName(const std::string& n);
    /** Sets the parameter description (explanatory comment string) */
    VT_EXPORT Parameter& setDescription(const std::string& d);
    /** Sets the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    VT_EXPORT Parameter& setUnit(const std::string& d);
    /** Sets the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    VT_EXPORT Parameter& setType(ParameterValue::ParameterType t);
    /** Sets a Parameter to be a tensor, specifying the shape at the same time */
    VT_EXPORT Parameter& setAsTensor(const std::vector<unsigned int>& shape);
    /** Returns whether the specified value is acceptable for the current Parameter (if it is constrained by range or enum list) */
    template<typename T>
    VT_EXPORT bool isValidNewValue(T t) const;
    /** Ensures that the default value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    VT_EXPORT bool ensureValidDefault();
    /** Ensures that the current value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    VT_EXPORT bool ensureValidCurrent();
    /** Returns value with any increment applied (starting at minimum, if specified)*/
    template<typename T>
    VT_EXPORT T enforceIncrement(T t);
    /** Sets the default value for the Parameter (Caveat: for scalars only) */
    template<typename T>
    VT_EXPORT Parameter& setDefault(T t);
    /** Sets the valid value range for the Parameter (only checked for numerical scalars) */
    template<typename T>
    VT_EXPORT Parameter& setRange(T mn, T mx);
    /** Unsets the valid value range for the Parameter; i.e. removes min and max bounds */
    VT_EXPORT Parameter& unsetRange();
    /** Sets the uniform increment (granularity) for the Parameter (only checked for numerical scalars) */
    template<typename T>
    VT_EXPORT Parameter& setIncrement(T t);
    /** Sets or overwrites the current value of the Parameter. Type compatibility is enforced and std::runtime_error is thrown for violations. */
    template<typename T>
    VT_EXPORT Parameter& setCurrent(T t);
    /** Copies over the current value from the specified Parameter into the present one */
    VT_EXPORT Parameter& setCurrentFrom(const Parameter& from);
    /** Copies over the current value from the default value, if specified */
    VT_EXPORT Parameter& setCurrentFromDefault();
    /** Specifies a vector of possible values and a vector of description strings, making the present Parameter an enum-style option list. Use zero-length arguments to remove an enum restriction. */
    template<typename T>
    VT_EXPORT Parameter& setOptions(const std::vector<T>& opts, const std::vector<std::string>& descriptions);
    /** Specifies an initializer_list of possible values and an initializer_list of description strings, making the present Parameter an enum-style option list */
    template<typename T>
    VT_EXPORT Parameter& setOptions(std::initializer_list<T> opts, std::initializer_list<std::string> descriptions);
    /** Returns the list of possible values for an enum-style Parameter (or an empty list for non-enums). */
    template<typename T>
    VT_EXPORT std::vector<T> getOptions() const;
    /** Returns the list of option descriptions for an enum-style Parameter (or an empty list for non-enums). */
    VT_EXPORT std::vector<std::string> getOptionDescriptions() const;
    /** Returns the ParameterValue representing the Parameters current or default value */
    VT_EXPORT ParameterValue getCurrentParameterValue();
    /** Returns the current value for the Parameter. Not supported for tensors (use getTensorData() instead). */
    template<typename T>
    VT_EXPORT T getCurrent() const;
    VT_EXPORT ParameterValue getDefaultParameterValue();
    /** Returns the default value for the Parameter. Not supported for tensors. */
    template<typename T>
    VT_EXPORT T getDefault() const;
    /** Returns the minimum value for the Parameter (or the lowest representable value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    VT_EXPORT T getMin() const;
    /** Returns the maximum value for the Parameter (or the highest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    VT_EXPORT T getMax() const;
    /** Returns the increment (granularity) for the Parameter (or the smallest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    VT_EXPORT T getIncrement() const;
    /** Returns true for an enum-style Parameter (i.e. a list of valid options has been set), false otherwise. */
    VT_EXPORT bool hasOptions() const;
    /** Returns true iff the Parameter has a defined current value */
    VT_EXPORT bool hasCurrent() const;
    /** Returns true iff the Parameter has a defined default value */
    VT_EXPORT bool hasDefault() const;
    /** Returns true iff the Parameter has a limited range set, false otherwise */
    VT_EXPORT bool hasRange() const;
    /** Returns true iff the Parameter has an increment (granularity) set, false otherwise */
    VT_EXPORT bool hasIncrement() const;
    /** Returns the x-th element of a 1-d tensor (vector). Also extends to higher dimensional tensors by indexing the flat data (as if getTensorData() has been used) */
    VT_EXPORT double at(unsigned int x);
    /** Returns the y-th row, x-th column data element for a 2-dimensional tensor. Fails for other shapes or types */
    VT_EXPORT double at(unsigned int y, unsigned int x);
    /** Returns the z-th slice, y-th row, x-th column data element for a 3-dimensional tensor. Fails for other shapes or types */
    VT_EXPORT double at(unsigned int z, unsigned int y, unsigned int x);
    // Header-only OpenCV glue:
#ifdef CV_MAJOR_VERSION
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Size object, yielding a tensor of shape {2} */
    VT_EXPORT Parameter& setTensorFromCvSize(const cv::Size& cvSize) {
        if ((getType()!=ParameterValue::TYPE_UNDEFINED) && !isTensor()) {
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
        setTensorData(data);
        setTensorDefaultData(data);
        return *this;
    }
    /** Sets an OpenCV Size object from a Tensor-type Parameter, which must be of shape {2}. */
    VT_EXPORT void setCvSizeFromTensor(cv::Size& cvSize) {
        if (getTensorNumElements() != 2) throw std::runtime_error("Parameter::setCvSizeFromTensor(): refused to export Tensor of size!=2 to cv::Size");
        cvSize = cv::Size((int) at(0), (int) at(1));
    }
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Mat object, yielding a two-dimensional tensor of identical shape */
    template<typename T>
    VT_EXPORT Parameter& setTensorFromCvMat(const cv::Mat_<T>& cvMat) {
        if ((getType()!=ParameterValue::TYPE_UNDEFINED) && !isTensor()) {
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
        setTensorData(data);
        setTensorDefaultData(data);
        return *this;
    }
    /** Sets an OpenCV Mat object of arbitrary base type from a Tensor-type Parameter, which must be two-dimensional. */
    template<typename T>
    VT_EXPORT void setCvMatFromTensor(cv::Mat_<T>& cvMat) {
        if (getTensorDimension() != 2) {
            std::ostringstream ss;
            ss << "{";
            for (unsigned int i=0; i<getTensorDimension(); ++i) {
                ss << getTensorShape()[i] << ", ";
            }
            ss << "}";
            ss << " " << getUid() << " " << ((int)getType());
            throw std::runtime_error(std::string("Parameter::Pimpl::setCvMatFromTensor(): refused to export non-2D Tensor to cv::Mat, offending shape is: ")+ss.str());
        }
        auto& refData = getTensorDataReference();
        cv::Mat_<T>(getTensorShape()[0], getTensorShape()[1], (T*)&refData[0]).copyTo(cvMat);
    }
#endif // CV_MAJOR_VERSION
};

} // namespace param
} // namespace visiontransfer

#endif


