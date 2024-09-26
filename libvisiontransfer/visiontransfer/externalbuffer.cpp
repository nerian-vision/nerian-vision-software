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
    inline int getNumParts() const { return parts.size(); }
    inline ExternalBuffer::Part getPart(int idx) const { return parts.at(idx); }
    inline unsigned char* getBufferPtr() const { return data; }
    inline size_t getBufferSize() const { return size; }
};

class ExternalBufferSet::Pimpl {
private:
    ImageSet::ExternalBufferHandle handle;
    std::vector<ExternalBuffer> buffers;
    bool ready;
public:
    Pimpl();
    Pimpl(const Pimpl& orig);
    void addBuffer(ExternalBuffer buf);
    inline ImageSet::ExternalBufferHandle getHandle() const { return handle; }
    inline int getNumBuffers() const { return buffers.size(); }
    inline ExternalBuffer getBuffer(int idx) const { return buffers.at(idx); }
    bool getReady() const { return ready; }
    void setReady(bool ready_) { ready = ready_; }
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

// ExternalBufferSet

ExternalBufferSet::Pimpl::Pimpl() {
    static ImageSet::ExternalBufferHandle nextBufferHandle = 1;
    handle = nextBufferHandle++;
    ready = false;
}

ExternalBufferSet::Pimpl::Pimpl(const ExternalBufferSet::Pimpl& orig) {
    handle = orig.handle;
    buffers = orig.buffers;
    ready = orig.ready;
}

void ExternalBufferSet::Pimpl::addBuffer(ExternalBuffer buf) {
    buffers.push_back(buf);
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

// ExternalBufferSet

ExternalBufferSet::ExternalBufferSet()
: pimpl(new ExternalBufferSet::Pimpl()) {
}

ExternalBufferSet::ExternalBufferSet(const ExternalBufferSet& orig)
: pimpl(new Pimpl(*(orig.pimpl))) {
}
const ExternalBufferSet& ExternalBufferSet::operator=(const ExternalBufferSet& from) {
    auto old = pimpl;
    pimpl = new Pimpl(*(from.pimpl));
    delete old;
    return *this;
}

ExternalBufferSet::~ExternalBufferSet() {
    delete pimpl;
}

ImageSet::ExternalBufferHandle ExternalBufferSet::getHandle() const {
    return pimpl->getHandle();
}

void ExternalBufferSet::addBuffer(ExternalBuffer buf) {
    pimpl->addBuffer(buf);
}

int ExternalBufferSet::getNumBuffers() const {
    return pimpl->getNumBuffers();
}

ExternalBuffer ExternalBufferSet::getBuffer(int idx) const {
    return pimpl->getBuffer(idx);
}

bool ExternalBufferSet::getReady() const {
    return pimpl->getReady();
}

void ExternalBufferSet::setReady(bool ready) {
    std::cout << "ExternalBufferSet #" << pimpl->getHandle() << " -> ready:=" << ready << std::endl;
    pimpl->setReady(ready);
}

} // namespace


