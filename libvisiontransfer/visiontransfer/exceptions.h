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

#ifndef VISIONTRANSFER_EXCEPTIONS_H
#define VISIONTRANSFER_EXCEPTIONS_H

#include <stdexcept>

namespace visiontransfer {

/**
 * \brief Exception class that is used for all protocol exceptions.
 */
class ProtocolException: public std::runtime_error {
public:
    ProtocolException(std::string msg): std::runtime_error(msg) {}
};

/**
 * \brief Exception class that is used for all transfer exceptions.
 */
class TransferException: public std::runtime_error {
public:
    TransferException(std::string msg): std::runtime_error(msg) {}
};

/**
 * \brief Exception class that is used for all parameter-related exceptions.
 */
class ParameterException: public std::runtime_error {
public:
    ParameterException(std::string msg): std::runtime_error(msg) {}
};

} // namespace

#endif
