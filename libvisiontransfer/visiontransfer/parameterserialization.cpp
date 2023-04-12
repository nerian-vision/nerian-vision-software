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

#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <iostream>

#include <visiontransfer/parameterserialization.h>
#include <visiontransfer/tokenizer.h>

namespace visiontransfer {
namespace internal {

using namespace param;

std::string escapeString(const std::string& str) {
    auto s = str;
    s = std::regex_replace(s, std::regex("\\\\"), "\\\\");
    s = std::regex_replace(s, std::regex("\\n"), "\\n");
    s = std::regex_replace(s, std::regex("\\t"), "\\t");
    return s;
}

std::string unescapeString(const std::string& str) {
    auto s = str;
    s = std::regex_replace(s, std::regex("([^\\\\])\\\\t"), "$1\t");
    s = std::regex_replace(s, std::regex("([^\\\\])\\\\n"), "$1\n");
    s = std::regex_replace(s, std::regex("\\\\\\\\"), "\\\\");
    return s;
}

// String serialization of full parameter info (line header "I") or metadata update (line header "M")
void ParameterSerialization::serializeParameterFullUpdate(std::stringstream& ss, const Parameter& param, const std::string& leader) {
    // 1  Uid
    ss << leader << "\t" << param.getUid() << "\t";
    // 2, 3  Access mode RW vs RO (for WebIf, then API) -- NOTE: this function should never even be invoked for an ACCESS_NONE param for the respective receiver
    switch (param.getAccessForConfig()) {
        case Parameter::ACCESS_READWRITE: ss << "2\t"; break;
        case Parameter::ACCESS_READONLY: ss << "1\t"; break;
        default: ss << "0\t";
    }
    switch (param.getAccessForApi()) {
        case Parameter::ACCESS_READWRITE: ss << "2\t"; break;
        case Parameter::ACCESS_READONLY: ss << "1\t"; break;
        default: ss << "0\t";
    }
    // 4  Interaction hint: parameter invisible/inactive/active
    ss << ((int) param.getInteractionHint()) << "\t";
    // 5  Modified flag (unsaved changes)
    ss << (param.getIsModified() ? "1" : "0") << "\t";
    // 6  Display name
    ss << param.getName() << "\t";
    // 7  Module name
    ss << param.getModuleName() << "\t";
    // 8  Category name
    ss << param.getCategoryName() << "\t";
    // 9  Type
    switch (param.getType()) {
        case ParameterValue::TYPE_INT:
            ss << "i\t";
            break;
        case ParameterValue::TYPE_DOUBLE:
            ss << "d\t";
            break;
        case ParameterValue::TYPE_BOOL:
            ss << "b\t";
            break;
        case ParameterValue::TYPE_STRING:
            ss << "s\t";
            break;
        case ParameterValue::TYPE_SAFESTRING:
            ss << "S\t";
            break;
        case ParameterValue::TYPE_TENSOR:
            ss << "T\t";
            break;
        case ParameterValue::TYPE_COMMAND:
            ss << "C\t";
            break;
        default:
            // should not happen
            ss << "?\t";
            break;
    }
    // 10  Unit
    ss << param.getUnit() << "\t";
    // 11  Description
    ss << escapeString(param.getDescription()) << "\t";
    // 12  Default value
    if (!param.isTensor()) {
        ss << param.getDefault<std::string>() << "\t";
    } else {
        auto shape = param.getTensorShape();
        ss << param.getTensorDimension() << " ";
        for (unsigned int i=0; i<param.getTensorDimension(); ++i) {
            ss << shape[i] << " ";
        }
        bool first=true;
        if (param.hasDefault()) {
            for (auto e: param.getTensorDefaultData()) {
                if (first) first = false;
                else ss << " ";
                ss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1) << e;
            }
        } else {
            // All-zero fallback
            for (int i=0; i<(int) param.getTensorNumElements(); ++i) {
                if (first) first = false;
                else ss << " ";
                ss << "0.0";
            }
        }
        ss << "\t";
    }
    // 13, 14, 15  Min, Max, Increment
    if (param.isScalar()) {
        if (param.hasRange()) {
            ss << param.getMin<std::string>() << "\t";
            ss << param.getMax<std::string>() << "\t";
        } else {
            ss << "\t\t";
        }
        if (param.hasIncrement()) {
            ss << param.getIncrement<std::string>() << "\t";
        } else {
            ss << "\t";
        }
    } else {
        ss << "\t\t\t"; // Tensor bounds undefined
    }
    // 16, 17  Option Values, Option Descriptions (;-separated lists)
    auto opts = param.getOptions<std::string>();
    for (unsigned int i=0; i<opts.size(); ++i) {
        if (i) ss << ";";
        ss << opts[i];
    }
    ss << "\t";
    auto descrs = param.getOptionDescriptions();
    for (unsigned int i=0; i<descrs.size(); ++i) {
        if (i) ss << ";";
        ss << descrs[i];
    }
    ss << "\t";
    // 18  Current value
    if (!param.isTensor()) {
        if (param.hasCurrent()) {
            ss << param.getCurrent<std::string>();
        } else {
            ss << param.getDefault<std::string>();
        }
    } else {
        auto shape = param.getTensorShape();
        ss << param.getTensorDimension() << " ";
        for (unsigned int i=0; i<param.getTensorDimension(); ++i) {
            ss << shape[i] << " ";
        }
        bool first=true;
        if (param.hasCurrent()) {
            for (auto e: param.getTensorData()) {
                if (first) first = false;
                else ss << " ";
                ss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1) << e;
            }
        } else {
            // All-zero fallback
            for (int i=0; i<(int) param.getTensorNumElements(); ++i) {
                if (first) first = false;
                else ss << " ";
                ss << "0.0";
            }
        }
    }
}

// expecting a tab-tokenization of a parameter info
Parameter ParameterSerialization::deserializeParameterFullUpdate(const std::vector<std::string>& toks, const std::string& leader) {
    static visiontransfer::internal::Tokenizer tokr;
    static visiontransfer::internal::Tokenizer tokrSemi;
    tokrSemi.separators({";"});
    if (toks.size() < 19) {
        throw std::runtime_error("deserializeParameterFullUpdate: parameter info string tokens missing");
    }
    // toks[0] should be "I" for full updates and "M" for metadata-only updates, but double-check
    if (toks[0] != std::string(leader)) throw std::runtime_error("deserializeParameterFullUpdate: attempted deserialization of a non-parameter");

    // 1  Param with UID (putting in place of reference)
    if (toks[1].size() < 1) throw std::runtime_error("deserializeParameterFullUpdate: malformed UID field");
    Parameter param = Parameter(toks[1]);
    // 2  Access rights, WebIf
    if (toks[2].size() != 1) throw std::runtime_error("deserializeParameterFullUpdate: malformed access field");
    if (toks[2]=="2") {
        param.setAccessForConfig(Parameter::ACCESS_READWRITE);
    } else if (toks[2]=="1") {
        param.setAccessForConfig(Parameter::ACCESS_READONLY);
    } else {
        param.setAccessForConfig(Parameter::ACCESS_NONE);
    }
    // 3  Access rights, API
    if (toks[3].size() != 1) throw std::runtime_error("deserializeParameterFullUpdate: malformed access field");
    if (toks[3]=="2") {
        param.setAccessForApi(Parameter::ACCESS_READWRITE);
    } else if (toks[3]=="1") {
        param.setAccessForApi(Parameter::ACCESS_READONLY);
    } else {
        param.setAccessForApi(Parameter::ACCESS_NONE);
    }
    // 4  Interaction hint: parameter invisible/inactive/active
    int hint = atol(toks[4].c_str());
    if ((hint<-1) || (hint>1)) {
        throw std::runtime_error("deserializeParameterFullUpdate: invalid interaction hint");
    }
    // 5  Modified flag (unsaved changes)
    param.setIsModified(toks[5] == "1");
    // 6, 7, 8  Display name, Module name, Category name
    param.setName(toks[6]);
    param.setModuleName(toks[7]);
    param.setCategoryName(toks[8]);
    // 9  Type
    if (toks[9].size() != 1) throw std::runtime_error("deserializeParameterFullUpdate: malformed type field");
    char typ = toks[9][0];
    bool isTensor = typ == 'T'; // for conditional processing of current value further down
    switch (typ) {
        case 'i': param.setType(ParameterValue::TYPE_INT); break;
        case 'd': param.setType(ParameterValue::TYPE_DOUBLE); break;
        case 'b': param.setType(ParameterValue::TYPE_BOOL); break;
        case 's': param.setType(ParameterValue::TYPE_STRING); break;
        case 'S': param.setType(ParameterValue::TYPE_SAFESTRING); break;
        case 'T': param.setType(ParameterValue::TYPE_TENSOR); break;
        case 'C': param.setType(ParameterValue::TYPE_COMMAND); break;
        default: throw std::runtime_error("deserializeParameterFullUpdate: unhandled type");
    }
    // 10, 11   Unit, Description
    param.setUnit(toks[10]);
    param.setDescription(unescapeString(toks[11]));
    // 12  Default value
    if (!isTensor) {
        param.setDefault<std::string>(toks[12]);
    }
    // 13, 14, 15  Min, Max, Increment
    if (param.isScalar()) {
        if ((toks[13].size()>0) && (toks[13]!="-") &&
            (toks[14].size()>0) && (toks[14]!="-")) {
            param.setRange<std::string>(toks[13], toks[14]);
        }
        if ((toks[15].size()>0) && (toks[15]!="-")) {
            param.setIncrement<std::string>(toks[15]);
        }
    }
    // 16, 17  Option values / descriptions, semicolon-separated
    if (!isTensor) {
        auto optvals = tokrSemi.tokenize(toks[16]);
        auto optdescrs = tokrSemi.tokenize(toks[17]);
        if ((optvals.size() > 0) && (optvals[0] != "")) {
            param.setOptions<std::string>(optvals, optdescrs);
        }
    }
    // 18  Current value
    if (!isTensor) {
        param.setCurrent<std::string>(toks[18]);
    } else {
        auto dataToks = tokr.tokenize(toks[18]);
        if (dataToks.size() < 1) {
            throw std::runtime_error("deserializeParameterFullUpdate: tensor with empty specification");
        } else {
            int dim = atol(dataToks[0].c_str());
            if (dataToks.size() < (unsigned long) 1+dim) {
                throw std::runtime_error("deserializeParameterFullUpdate: tensor with incomplete specification");
            } else {
                std::vector<unsigned int> shape;
                for (int i=0; i<dim; ++i) {
                    shape.push_back((unsigned int) atol(dataToks[1+i].c_str()));
                }
                param.setAsTensor(shape);
                int tensorsize = param.getTensorNumElements();
                if (dataToks.size() != (unsigned long) tensorsize + 1 + dim) {
                    throw std::runtime_error("deserializeParameterFullUpdate: tensor with mismatching data size");
                } else {
                    std::vector<double> data;
                    for (int i=0; i<tensorsize; ++i) {
                        data.push_back(atof(dataToks[i+1+dim].c_str()));
                    }
                    param.setTensorData(data);
                }
            }
        }
    }
    return param;
}

// String serialization of current-value-only modification (line header "V")
void ParameterSerialization::serializeParameterValueChange(std::stringstream& ss, const Parameter& param) {
    if (param.isScalar()) {
        ss << "V" << "\t" << param.getUid() << "\t" << (param.getIsModified()?"1":"0") << "\t" << param.getCurrent<std::string>();
    } else {
        // Tensor with current shape + data
        ss << "V" << "\t" << param.getUid() << "\t" << (param.getIsModified()?"1":"0") << "\t";
        auto shape = param.getTensorShape();
        ss << param.getTensorDimension() << " ";
        for (unsigned int i=0; i<param.getTensorDimension(); ++i) {
            ss << shape[i] << " ";
        }
        bool first=true;
        for (auto e: param.getTensorData()) {
            if (first) first = false;
            else ss << " ";
            ss << std::setprecision(std::numeric_limits<double>::max_digits10 - 1) << e;
        }
    }
}

void ParameterSerialization::deserializeParameterValueChange(const std::vector<std::string>& toks, Parameter& param) {
    static visiontransfer::internal::Tokenizer tokr;
    if (toks.size() < 4) throw std::runtime_error("deserializeParameterValueChange: incomplete data");
    // toks[0] should be "V", but double-check
    if (toks[0] != "V") throw std::runtime_error("deserializeParameterValueChange: cannot deserialize, not a value change");
    if (toks[1] != param.getUid()) throw std::runtime_error("deserializeParameterValueChange: UID mismatch (bug)");
    param.setIsModified(toks[2] == "1");
    if (param.isTensor()) {
        auto dataToks = tokr.tokenize(toks[3]);
        if (dataToks.size() < 1) {
            throw std::runtime_error("deserializeParameterValueChange: tensor with empty specification");
        } else {
            int dim = atol(dataToks[0].c_str());
            if (dataToks.size() < (unsigned long) 1+dim) {
                throw std::runtime_error("deserializeParameterValueChange: tensor with incomplete specification");
            } else {
                // Verify shape
                int tensorsize = param.getTensorNumElements();
                int elems = 1;
                for (int i=0; i<dim; ++i) {
                    elems *= (unsigned int) atol(dataToks[1+i].c_str());
                }
                if (elems != tensorsize) {
                    // Mismatch between reported data shape and known shape
                    throw std::runtime_error("deserializeParameterValueChange: tensor with mismatching shape");
                }
                if (dataToks.size() != (unsigned long) tensorsize + 1 + dim) {
                    // Insufficient or extraneous data elements
                    throw std::runtime_error("deserializeParameterValueChange: tensor with mismatching data size");
                } else {
                    std::vector<double> data;
                    for (int i=0; i<tensorsize; ++i) {
                        data.push_back(atof(dataToks[i+1+dim].c_str()));
                    }
                    param.setTensorData(data);
                }
            }
        }
    } else {
        param.setCurrent<std::string>(toks[3]);
    }
}

void ParameterSerialization::serializeAsyncResult(std::stringstream& ss, const std::string& requestId, bool success, const std::string& message) {
    ss << "R\t" << requestId << "\t" << (success?"1\t":"0\t") << message;
}

void ParameterSerialization::deserializeAsyncResult(const std::vector<std::string>& toks, std::string& requestId, bool& success, std::string& message) {
    if (toks.size() < 4) throw std::runtime_error("deserializeAsyncResult: incomplete data");
    if (toks[0] != "R") throw std::runtime_error("deserializeAsyncResult: cannot deserialize, not an async result");
    requestId = toks[1];
    success = (toks[2] == "1");
    message = toks[3];
}

} // namespace internal
} // namespace visiontransfer

