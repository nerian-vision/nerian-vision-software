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


#ifndef NERIAN_XMLFILES_H
#define NERIAN_XMLFILES_H

#include <map>
#include <string>

namespace GenTL {

// THE IMPLEMENTATION FOR THIS CLASS IS AUTO GENERATED!
// Receives the contents of xml files that are linked into the GenTL
// producer.
class XmlFiles {
public:
    static const std::string& getFileContent(const std::string name) {
        std::map<std::string, FileData>::iterator iter =
            xmlFiles.find(name);
        if(iter == xmlFiles.end()) {
            static std::string emptyString = "";
            return emptyString;
        } else {
            return iter->second.content;
        }
    }

    static void getFileVersion(const std::string name, int& majorVer, int& minorVer, int& subminorVer) {
        std::map<std::string, FileData>::iterator iter =
            xmlFiles.find(name);
        if(iter == xmlFiles.end()) {
            majorVer = 0;
            minorVer = 0;
            subminorVer = 0;
        } else {
            majorVer = iter->second.majorVer;
            minorVer = iter->second.minorVer;
            subminorVer = iter->second.subminorVer;
        }
    }

private:
    struct FileData {
        int majorVer;
        int minorVer;
        int subminorVer;
        std::string content;

        FileData(int majorVer, int minorVer, int subminorVer, const char* content)
            : majorVer(majorVer), minorVer(minorVer), subminorVer(subminorVer), content(content) {
        }
    };

    static std::map<std::string, FileData> xmlFiles;
};

}
#endif
