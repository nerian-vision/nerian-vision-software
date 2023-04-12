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

#ifndef VISIONTRANSFER_PARAMETERSERIALIZATION_H
#define VISIONTRANSFER_PARAMETERSERIALIZATION_H

#include <vector>
#include <string>
#include <sstream>

#include <visiontransfer/parametervalue.h>
#include <visiontransfer/parameter.h>
#include <visiontransfer/parameterset.h>

namespace visiontransfer {
namespace internal {

/// This is the common [de]serialization filter for all of nvparam's external network protocols.
class ParameterSerialization {
    public:
        // Note: the [de]serialization of metadata-only updates uses the same format as the full update (and just ignores the value part)
        static void serializeParameterFullUpdate(std::stringstream& ss, const param::Parameter& param, const std::string& leader="I");
        static param::Parameter deserializeParameterFullUpdate(const std::vector<std::string>& toks, const std::string& leader="I");
        static void serializeParameterValueChange(std::stringstream& ss, const param::Parameter& param);
        static void deserializeParameterValueChange(const std::vector<std::string>& toks, param::Parameter& param);
        static void serializeAsyncResult(std::stringstream& ss, const std::string& requestId, bool success, const std::string& message);
        static void deserializeAsyncResult(const std::vector<std::string>& toks, std::string& requestId, bool& success, std::string& message);
};

} // namespace internal
} // namespace visiontransfer

#endif


