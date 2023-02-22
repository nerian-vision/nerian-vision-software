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

#ifndef VISIONTRANSFER_PARAMETERTRANSFERDATA_H
#define VISIONTRANSFER_PARAMETERTRANSFERDATA_H

namespace visiontransfer {
namespace internal {

#pragma pack(push,1)
struct TransportParameterInfo {
    uint32_t id = 0;
    uint32_t type = 0;
    uint32_t flags = 0;
    ParameterInfo::ParameterValue value = {0};
    ParameterInfo::ParameterValue min = {0};
    ParameterInfo::ParameterValue max = {0};
    ParameterInfo::ParameterValue inc = {0};
};
#pragma pack(pop)

}} // namespace

#endif

