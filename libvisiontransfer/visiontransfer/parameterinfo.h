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

#ifndef VISIONTRANSFER_STANDARDPARAMETERS_H
#define VISIONTRANSFER_STANDARDPARAMETERS_H

#include <map>
#include <string>
#include "visiontransfer/common.h"

namespace visiontransfer {

class VT_EXPORT ParameterInfo {
public:
    union ParameterValue {
        int32_t intVal;
        bool boolVal;
        double doubleVal;
    };

    enum ParameterType {
        TYPE_INT                                 = 1,
        TYPE_DOUBLE                              = 2,
        TYPE_BOOL                                = 3,
    };

    ParameterInfo();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    // For internal use only
    static ParameterInfo fromInt(const std::string& name, bool writeable,
            int value, int min = -1, int max = -1, int inc = -1);
    static ParameterInfo fromDouble(const std::string& name, bool writeable,
            double value, double min = -1, double max = -1, double inc = -1);
    static ParameterInfo fromBool(const std::string& name, bool writeable, bool value);
#endif

    /**
     * \brief Returns the string representation of the parameter name
     */
    std::string getName() const;
    /**
     * \brief Returns the type of the parameter
     */
    ParameterType getType() const;
    /**
     * \brief Returns whether the parameter is writeable (or read-only)
     */
    bool isWriteable() const;
    /**
     * \brief Returns the current parameter value, cast to the desired type (int, double or bool)
     */
    template<typename T> T getValue() const;
    /**
     * \brief Returns the minimum parameter value, cast to the desired type (int, double or bool)
     */
    template<typename T> T getMin() const;
    /**
     * \brief Returns the maximum parameter value, cast to the desired type (int, double or bool)
     */
    template<typename T> T getMax() const;
    /**
     * \brief Returns the increment of the parameter (i.e. increment for raising / lowering the value), cast to the desired type (int, double or bool)
     */
    template<typename T> T getInc() const;

private:
    class Pimpl;
    Pimpl* pimpl;
};

} // namespace

#endif

