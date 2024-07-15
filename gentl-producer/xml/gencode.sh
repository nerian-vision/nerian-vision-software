#!/bin/bash

mkdir -p gen

# Top part of the header file
cat > gen/xmlfiles.cpp << EOF
#include "misc/xmlfiles.h"

// THIS CODE IS AUTO GENERATED! DO NOT CHANGE!

namespace GenTL {

std::map<std::string, XmlFiles::FileData> XmlFiles::xmlFiles =  {

EOF

for file in *.xml; do
    echo -e "\n$file"

    # We use the current date as version numbers
    major=`date -r $file +'%-y'`
    minor=`date -r $file  +'%-j'`
    subminor=`date -r $file  +'%-s'`
    ((subminor = subminor % 86400))

    # Construct a fake GUID from the file's md5sum
    guid=`md5sum $file | \
        awk '{print substr($1,0,8) "-" substr($1,9,4) "-" substr($1,13,4) "-" substr($1,17,4) "-" substr($1,21,12)}'`

    # Set GUID
    sed -e "s/\$guid/$guid/" $file | sed -e "s/\$major/$major/" |
        sed -e "s/\$minor/$minor/" | sed -e "s/\$subminor/$subminor/" > gen/temp.xml

    # Check syntax
    xmllint --noout --schema GenApiSchema_Version_1_1.xsd gen/temp.xml || exit 1

    # Escape string and write to header (in 100-line std::string chunks, for the sake of MSVC)
    echo -en "{\"${file}\", FileData($major, $minor, $subminor, std::string(\n" >> gen/xmlfiles.cpp
    cat gen/temp.xml | sed -ze 's/\n/\\n\n/g' | sed -e 's/\"/\\\"/g' > .xml.tmp
    i=0
    while IFS= read -r line; do
        i=$(($i+1))
        if [ $i -gt 100 ]; then
            i=0
            echo "    ) + std::string(" >> gen/xmlfiles.cpp
        fi
        printf '%s\n' "    \"$line\"" >> gen/xmlfiles.cpp
    done < .xml.tmp
    rm -f .xml.tmp
    echo -e "))},\n\n" >> gen/xmlfiles.cpp

    rm gen/temp.xml
done

# Write bottom part
cat >> gen/xmlfiles.cpp << EOF
};
}
EOF
