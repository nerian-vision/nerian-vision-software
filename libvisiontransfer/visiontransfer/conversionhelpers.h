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

namespace visiontransfer {
namespace internal {

/// Internal helper functions for type conversion
class VT_EXPORT ConversionHelpers {
public:
    ///Converts any type to a double
    template<typename T>
    static double anyToDouble(T val);

    ///Converts any type to a string
    template<typename T>
    static std::string anyToString(T val) {
        std::ostringstream ss;
        ss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1) << val;
        return ss.str();
    }

    // Wrap anything in to_string if string is expected, or cast for numbers
    template<typename S, typename T, typename std::enable_if<std::is_arithmetic<S>::value>::type* = nullptr >
    static S toStringIfStringExpected(T val) {
        return static_cast<S>(val);
    }
    template<typename S, typename T, typename std::enable_if<std::is_same<S, std::string>::value>::type* = nullptr  >
    static S toStringIfStringExpected(T val) {
        return std::to_string(val);
    }
};

template<> inline double ConversionHelpers::anyToDouble(const std::string& val) { return atol(val.c_str()); }
template<> inline double ConversionHelpers::anyToDouble(std::string val) { return atol(val.c_str()); }
template<> inline double ConversionHelpers::anyToDouble(const char* val) { return atol(val); }
template<typename T> inline double ConversionHelpers::anyToDouble(T val) { return (double)val; }

} // namespace internal
} // namespace visiontransfer
