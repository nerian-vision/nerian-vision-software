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

#include <cstdio>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <thread>
#include "visiontransfer/imageset.h"
#include "visiontransfer/externalbuffer.h"

using namespace visiontransfer;

namespace visiontransfer {

class ExternalBuffer::Pimpl {
private:
    // The underlying buffer, backed by *externally allocated* memory
    unsigned char* data;
    // The length of available allocated memory
    size_t size;
    // The parts that can be packed into the buffer, with rules for
    // conversion and filling missing data
    std::vector<Part> parts;
public:
    Pimpl(unsigned char* data, size_t size);
    Pimpl(const Pimpl& orig);
    void appendPartDefinition(ExternalBuffer::Part part);
    inline int getNumParts() const { return (int) parts.size(); }
    inline ExternalBuffer::Part getPart(int idx) const { return parts.at(idx); }
    inline unsigned char* getBufferPtr() const { return data; }
    inline size_t getBufferSize() const { return size; }
};

// Pimpl functions

// ExternalBuffer

ExternalBuffer::Pimpl::Pimpl(unsigned char* data_, size_t size_) {
    data = data_;
    size = size_;
}

ExternalBuffer::Pimpl::Pimpl(const ExternalBuffer::Pimpl& orig) {
    data = orig.data;
    size = orig.size;
    parts = orig.parts;
}

void ExternalBuffer::Pimpl::appendPartDefinition(ExternalBuffer::Part part) {
    parts.push_back(part);
}

//
// Public API implementation
//

// ExternalBuffer

ExternalBuffer::ExternalBuffer(unsigned char* data, size_t size)
: pimpl(new ExternalBuffer::Pimpl(data, size)) {
}

ExternalBuffer::ExternalBuffer(const ExternalBuffer& orig)
: pimpl(new Pimpl(*(orig.pimpl))) {
}

ExternalBuffer::~ExternalBuffer() {
    delete pimpl;
}

void ExternalBuffer::appendPartDefinition(Part part) {
    pimpl->appendPartDefinition(part);
}

int ExternalBuffer::getNumParts() const {
    return pimpl->getNumParts();
}

ExternalBuffer::Part ExternalBuffer::getPart(int idx) const {
    return pimpl->getPart(idx);
}

unsigned char* ExternalBuffer::getBufferPtr() const {
    return pimpl->getBufferPtr();
}

size_t ExternalBuffer::getBufferSize() const {
    return pimpl->getBufferSize();
}

} // namespace


