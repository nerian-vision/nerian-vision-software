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
class VT_EXPORT Parameter {

private:
    // We (mostly) follow the pimpl idiom here
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
    Parameter();
    /** Generate a new Parameter with specified UID (preferred) for manual filling */
    Parameter(const std::string& uid);
    /** Generate a copy of a Parameter object */
    Parameter(const Parameter& other);
    /** Destructor */
    ~Parameter();
    /** Assignment copy operator */
    Parameter& operator= (const Parameter& other);
    /** Return the current UID */
    std::string getUid() const;
    /** Return the current human-readable name */
    std::string getName() const;
    /** Return the overarching module name (abstract categorization for GUI etc.) */
    std::string getModuleName() const;
    /** Return the finer-grained category name within a module */
    std::string getCategoryName() const;
    /** Return the parameter description (explanatory comment string) */
    std::string getDescription() const;
    /** Return the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    std::string getUnit() const;
    /** Return the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    ParameterValue::ParameterType getType() const;
    /** Return the visibility / access mode for the configuration web interface */
    ParameterAccessMode getAccessForConfig() const;
    /** Return the visibility / access mode for the external API bridge */
    ParameterAccessMode getAccessForApi() const;
    /** Sets the visibility / access mode for the configuration web interface */
    Parameter& setAccessForConfig(ParameterAccessMode mode);
    /** Sets the visibility / access mode for the external API bridge */
    Parameter& setAccessForApi(ParameterAccessMode mode);
    /** Gets the interaction UI hint (invisible / inactive / active) */
    ParameterInteractionHint getInteractionHint() const;
    /** Sets the interaction UI hint (invisible / inactive / active) */
    Parameter& setInteractionHint(ParameterInteractionHint hint);
    /** Returns whether the Parameter has unsaved (and unreverted) run-time modifications to its previously saved value, according to the parameter server. */
    bool getIsModified() const;
    /** Returns true iff the tensor is only updated when polled. Such parameters are not synchronized in the background and must be polled (in the backend outside this class) to obtain their up-to-date values. */
    bool getIsPolled() const;
    /** Sets the polled-updates-only flag. This is controlled by the device; changing it has no effect in client-side code. */
    Parameter& setIsPolled(bool mod);
    /** Sets the runtime-modified flag. This is controlled by the device; changing it has no effect in client-side code. */
    Parameter& setIsModified(bool mod);
    /** Return the governor type (whether setting the parameter is controlled by a script, D-Bus, or nothing yet) */
    GovernorType getGovernorType() const;
    /** Return the governor string, i.e. the specific script cmdline or the controlling D-Bus name */
    std::string getGovernorString() const;
    /** Sets the parameter governor, a script or D-Bus name that controls setting the parameter */
    Parameter& setGovernor(GovernorType govType, const std::string& govStr);
    /** Sets the governor, a script or D-Bus name that controls setting the parameter */
    Parameter& setGovernorPollString(const std::string& govStr);
    /** Gets the oninit action (after first load). False = do nothing; true = perform the same action as on change. Used for calling governor scripts immediately when the parameter daemon initializes. */
    bool getInvokeGovernorOnInit() const;
    /** Sets the oninit action (see getInvokeGovernorOnInit). */
    Parameter& setInvokeGovernorOnInit(bool invoke);
    /** Perform a substition of %-initiated placeholders with correctly
        quoted parameter [meta-]data (for compiling shell commandlines).
    */
    std::string interpolateCommandLine(const ParameterValue& newVal, GovernorFunction fn = GOVERNOR_FN_CHANGE_VALUE);
    /** Returns true iff the value type of the Parameter is TENSOR */
    bool isTensor() const;
    /** Returns true iff the value type of the Parameter is scalar, i.e. neither TENSOR nor COMMAND */
    bool isScalar() const;
    /** Returns true iff the value type of the Parameter is COMMAND (a special string type that corresponds to single-shot actions such as buttons) */
    bool isCommand() const;
    /** Returns the tensor dimension, i.e. 1 for vectors, 2 for matrices etc. */
    unsigned int getTensorDimension() const;
    /** Returns the tensor shape in all dimensions. This is {rows, columns} for matrices. */
    std::vector<unsigned int> getTensorShape() const;
    /** Returns the total number of elements in the Tensor (product of the sizes in all dimensions). */
    unsigned int getTensorNumElements() const;
    /** Returns a flat copy of the tensor data as a vector<double> */
    std::vector<double> getTensorData() const;
    /** Returns a flat copy of the tensor default data as a vector<double> (mostly internal use) */
    std::vector<double> getTensorDefaultData() const;
    /** Returns a reference to the tensor data (CAUTION) */
    std::vector<double>& getTensorDataReference();
    /** Returns a reference to the tensor default data (CAUTION) */
    std::vector<double>& getTensorDefaultDataReference();
    /** Sets the tensor data from a flat representation */
    Parameter& setTensorData(const std::vector<double>& data);
    /** Sets the fallback tensor data */
    Parameter& setTensorDefaultData(const std::vector<double>& data);
#ifdef CV_MAJOR_VERSION
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Size object, yielding a tensor of shape {2} */
    Parameter& setTensorFromCvSize(const cv::Size& cvSize);
    /** Sets an OpenCV Size object from a Tensor-type Parameter, which must be of shape {2}. */
    void setCvSizeFromTensor(cv::Size& cvSize);
    /** Sets a Tensor-type (or still undefined) Parameter from an OpenCV Mat object, yielding a two-dimensional tensor of identical shape */
    template<typename T>
    Parameter& setTensorFromCvMat(const cv::Mat_<T>& cvMat);
    /** Sets an OpenCV Mat object of arbitrary base type from a Tensor-type Parameter, which must be two-dimensional. */
    template<typename T>
    void setCvMatFromTensor(cv::Mat_<T>& cvMat);
#endif // CV_MAJOR_VERSION
    /** Sets the human-readable name */
    Parameter& setName(const std::string& name);
    /** Sets the overarching module name (abstract categorization for GUI etc.) */
    Parameter& setModuleName(const std::string& n);
    /** Sets the finer-grained category name within a module */
    Parameter& setCategoryName(const std::string& n);
    /** Sets the parameter description (explanatory comment string) */
    Parameter& setDescription(const std::string& d);
    /** Sets the reference unit for the values of the Parameter as a string (e.g. "cm", "µs" etc.) */
    Parameter& setUnit(const std::string& d);
    /** Sets the type of the parameter values, one of the scalar, tensor, or command types from ParameterValue::ParameterType. Value assignments are subject to type compatibility checks. */
    Parameter& setType(ParameterValue::ParameterType t);
    /** Sets a Parameter to be a tensor, specifying the shape at the same time */
    Parameter& setAsTensor(const std::vector<unsigned int>& shape);
    /** Returns whether the specified value is acceptable for the current Parameter (if it is constrained by range or enum list) */
    template<typename T>
    bool isValidNewValue(T t) const;
    /** Ensures that the default value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    bool ensureValidDefault();
    /** Ensures that the current value is in the valid options (if set) or range (if set). Returns whether value was revised. */
    bool ensureValidCurrent();
    /** Returns value with any increment applied (starting at minimum, if specified)*/
    template<typename T> T enforceIncrement(T t);
    /** Sets the default value for the Parameter (Caveat: for scalars only) */
    template<typename T>
    Parameter& setDefault(T t);
    /** Sets the valid value range for the Parameter (only checked for numerical scalars) */
    template<typename T>
    Parameter& setRange(T mn, T mx);
    /** Unsets the valid value range for the Parameter; i.e. removes min and max bounds */
    Parameter& unsetRange();
    /** Sets the uniform increment (granularity) for the Parameter (only checked for numerical scalars) */
    template<typename T>
    Parameter& setIncrement(T t);
    /** Sets or overwrites the current value of the Parameter. Type compatibility is enforced and std::runtime_error is thrown for violations. */
    template<typename T>
    Parameter& setCurrent(T t);
    /** Copies over the current value from the specified Parameter into the present one */
    Parameter& setCurrentFrom(const Parameter& from);
    /** Copies over the current value from the default value, if specified */
    Parameter& setCurrentFromDefault();
    /** Specifies a vector of possible values and a vector of description strings, making the present Parameter an enum-style option list. Use zero-length arguments to remove an enum restriction. */
    template<typename T>
    Parameter& setOptions(const std::vector<T>& opts, const std::vector<std::string>& descriptions);
    /** Specifies an initializer_list of possible values and an initializer_list of description strings, making the present Parameter an enum-style option list */
    template<typename T>
    Parameter& setOptions(std::initializer_list<T> opts, std::initializer_list<std::string> descriptions);
    /** Returns the list of possible values for an enum-style Parameter (or an empty list for non-enums). */
    template<typename T>
    std::vector<T> getOptions() const;
    /** Returns the list of option descriptions for an enum-style Parameter (or an empty list for non-enums). */
    std::vector<std::string> getOptionDescriptions() const;
    /** Returns the ParameterValue representing the Parameters current or default value */
    ParameterValue getCurrentParameterValue();
    /** Returns the current value for the Parameter. Not supported for tensors (use getTensorData() instead). */
    template<typename T>
    T getCurrent() const;
    ParameterValue getDefaultParameterValue();
    /** Returns the default value for the Parameter. Not supported for tensors. */
    template<typename T>
    T getDefault() const;
    /** Returns the minimum value for the Parameter (or the lowest representable value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getMin() const;
    /** Returns the maximum value for the Parameter (or the highest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getMax() const;
    /** Returns the increment (granularity) for the Parameter (or the smallest possible value for the inferred template type if unset). Not supported for tensors. */
    template<typename T>
    T getIncrement() const;
    /** Returns true for an enum-style Parameter (i.e. a list of valid options has been set), false otherwise. */
    bool hasOptions() const;
    /** Returns true iff the Parameter has a defined current value */
    bool hasCurrent() const;
    /** Returns true iff the Parameter has a defined default value */
    bool hasDefault() const;
    /** Returns true iff the Parameter has a limited range set, false otherwise */
    bool hasRange() const;
    /** Returns true iff the Parameter has an increment (granularity) set, false otherwise */
    bool hasIncrement() const;
    /** Returns the x-th element of a 1-d tensor (vector). Also extends to higher dimensional tensors by indexing the flat data (as if getTensorData() has been used) */
    double at(unsigned int x);
    /** Returns the y-th row, x-th column data element for a 2-dimensional tensor. Fails for other shapes or types */
    double at(unsigned int y, unsigned int x);
    /** Returns the z-th slice, y-th row, x-th column data element for a 3-dimensional tensor. Fails for other shapes or types */
    double at(unsigned int z, unsigned int y, unsigned int x);
};

} // namespace param
} // namespace visiontransfer

#endif


