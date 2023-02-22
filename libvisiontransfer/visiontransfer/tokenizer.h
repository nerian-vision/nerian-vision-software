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

#ifndef VISIONTRANSFER_TOKENIZER_H
#define VISIONTRANSFER_TOKENIZER_H

#include <vector>
#include <string>
#include <sstream>

namespace visiontransfer {
namespace internal {

class Tokenizer {
public:
    Tokenizer(): _separators{" ", "\t"}, _comment_initiators{"#"}, _strip_chars(""), _collapse(true), _quoting(true) {}
    Tokenizer& separators(const std::vector<std::string>& seps) { _separators = seps; return *this; }
    Tokenizer& strip_chars(const std::string& chars) { _strip_chars = chars; return *this; }
    Tokenizer& collapse(bool coll) { _collapse = coll; return *this; }
    Tokenizer& quoting(bool quot) { _quoting = quot; return *this; }
#if __cplusplus >= 201703L
    std::vector<std::string> tokenize(const std::string_view& inp) {
#else
    std::vector<std::string> tokenize(const std::string& inp) {
#endif
        std::vector<std::string> toks;
        std::stringstream ss;
        bool issep, iscomment;
        char quotemode = '\0';
        for (size_t i=0; i<inp.size(); ++i) {
            if (quotemode == '\0') {
                // in unquoted region
                issep = false; iscomment = false;
                for (const auto& comm: _comment_initiators) {
                    if (inp.substr(i, comm.size())==comm) {
                        iscomment = true;
                        i += comm.size()-1;
                        break;
                    }
                }
                for (const auto& sep: _separators) {
                    if (inp.substr(i, sep.size())==sep) {
                        issep = true;
                        i += sep.size()-1;
                        break;
                    }
                }
                if (iscomment) {
                    i = inp.size();
                    break; // end of processing
                } else if (issep) {
                    std::string tmp = ss.str();
                    if (!_collapse || !tmp.empty()) {
                        toks.push_back(tmp);
                        ss.str("");
                    }
                } else if (inp[i] == '"') {
                    quotemode = '"';
                } else if (inp[i] == '\'') {
                    quotemode = '\'';
                } else {
                    ss << inp[i];
                }
            } else if (quotemode == '\'') {
                // in single quote mode
                if (inp.substr(i, 2) == "\\\'") {
                    ss << '\'';
                    i += 1;
                } else if (inp.substr(i, 2) == "\\\\") {
                    ss << '\\';
                    i += 1;
                } else if (inp[i] == '\'') {
                    toks.push_back(ss.str());
                    ss.str("");
                    quotemode = '\0';
                } else {
                    ss << inp[i];
                }
            } else if (quotemode == '"') {
                // in double quote mode
                if (inp.substr(i, 2) == "\\\"") {
                    ss << '"';
                    i += 1;
                } else if (inp.substr(i, 2) == "\\\\") {
                    ss << '\\';
                    i += 1;
                } else if (inp.substr(i, 2) == "\\n") {
                    ss << '\n';
                    i += 1;
                } else if (inp[i] == '"') {
                    toks.push_back(ss.str());
                    ss.str("");
                    quotemode = '\0';
                } else {
                    ss << inp[i];
                }
            }
        }
        std::string tmp = ss.str();
        if (!_collapse || !tmp.empty()) {
            toks.push_back(tmp);
        }
        if (_strip_chars.size()) {
            std::vector<std::string> toks2;
            for (auto s: toks) {
                auto st = s.find_first_not_of(_strip_chars);
                auto en = s.find_last_not_of(_strip_chars);
                toks2.push_back((st==en)?std::string():std::string(s.substr(st, en+1-st)));
            }
            return toks2;
        } else {
            return toks;
        }
    }
protected:
    std::vector<std::string> _separators;
    std::vector<std::string> _comment_initiators;
    std::string _strip_chars;
    bool _collapse, _quoting;
};

} // namespace internal
} // namespace visiontransfer

#endif

