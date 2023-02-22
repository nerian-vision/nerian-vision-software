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

#ifndef NERIAN_HANDLE_H
#define NERIAN_HANDLE_H

#include "misc/common.h"
#include <genicam/gentl.h>

namespace GenTL {

/*
 *  Base class for all handle types.
 */
class Handle {
public:
    enum HandleType {
        TYPE_SYSTEM,
        TYPE_BUFFER,
        TYPE_STREAM,
        TYPE_DEVICE,
        TYPE_EVENT,
        TYPE_PORT,
        TYPE_INTERFACE
    };

    Handle(HandleType type): type(type) {
    }

    HandleType getType() const {
        return type;
    }

private:
    HandleType type;
};

}
#endif
