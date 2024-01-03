/*******************************************************************************
 * Copyright (c) 2023 Allied Vision Technologies GmbH
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

#include "visiontransfer/internalinformation.h"

using namespace visiontransfer;
using namespace visiontransfer::internal;

namespace visiontransfer {
namespace internal {

const char InternalInformation::DISCOVERY_BROADCAST_MSG_LEGACY[16]  = "NERIAN-DISCOVER";
const char InternalInformation::DISCOVERY_BROADCAST_MSG[25] = "NERIAN-DISCOVER-EXTENDED";

}} // namespace

