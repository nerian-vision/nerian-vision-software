#!/bin/bash
if [ $# -ne 1 ]; then
    echo "Usage: $0 VERSION"
    exit 1
fi

ver=$1

# Update version number
sed -i "s/PROJECT_NUMBER = .*/PROJECT_NUMBER = $ver/" doxygen.in
sed -i "s/^Vision Transfer Library .*/Vision Transfer Library $ver/" README.md

doxygen doxygen.in

# Add statcounter code
for file in `grep -l nerian.png doc/html/*.html`; do

sed -i 's/<\/head>/\
<script>\
    if (top.location == self.location) {\
        if(window.location.hostname == "nerian.de") {\
            top.location = "\/support\/dokumentationen\/api-doc\/?" + location.href.replace(\/^.*\\\/\\\/[^\\\/]+\\\/\/, ""); \
        } else {\
            top.location = "\/support\/documentation\/api-doc\/?" + location.href.replace(\/^.*\\\/\\\/[^\\\/]+\\\/\/, ""); \
        }\
    }\
<\/script>\
<\/head>\
/' $file || exit 1

sed -i 's/<a /<a target=\"_parent\" /' $file || exit 1

sed -i 's/\(^.*nerian\.png.*$\)/\1\
\
<!-- Start of StatCounter Code for Default Guide -->\
<script type=\"text\/javascript\">\
var sc_project=10301376;\
var sc_invisible=1;\
var sc_security=\"54e2c38d\";\
var scJsHost = ((\"https:\" == document.location.protocol) ?\
\"https:\/\/secure.\" : \"http:\/\/www.\");\
document.write(\"<sc\"+\"ript type=\\\"text\/javascript\\\" src=\\\"\" +\
scJsHost+\
\"statcounter.com\/counter\/counter.js\\\"><\/\"+\"script>\");\
<\/script>\
<noscript><div class=\"statcounter\"><a title=\"shopify\
analytics\" href=\"http:\/\/statcounter.com\/shopify\/\"\
target=\"_blank\"><img class=\"statcounter\"\
src=\"http:\/\/c.statcounter.com\/10301376\/0\/54e2c38d\/1\/\"\
alt=\"shopify analytics\"><\/a><\/div><\/noscript>\
<!-- End of StatCounter Code for Default Guide -->\
/' $file || exit 1
done

rsync -cav --no-t --delete doc/ -e ssh web45@s177.goserver.host:/home/www/nerian-wordpress/nerian-content/api-doc

# Rebuild python module
pushd ../client-sw/libvisiontransfer-client/build
rm ../python3-wheel/*.whl
make || exit 1

# Install and export documentation
python3 -m pip install ../python3-wheel/*.whl || exit 1
python3 -m pydoc -w visiontransfer || exit 1
echo y | python3 -m pip uninstall visiontransfer || exit 1

# fix links
sed -i 's/<a href=\"file:.*<\/a>//' visiontransfer.html || exit 1
sed -i 's/<a href=".">index<\/a>//' visiontransfer.html || exit 1
sed -i 's/<a href="visiontransfer.html/<a target=\"_parent\" href="/' visiontransfer.html || exit 1
sed -i 's/<a href="[^#][^"]*"/<a /' visiontransfer.html || exit 1

sed -i 's/<\/head>/\
<script>\
    if (top.location == self.location) {\
        if(window.location.hostname == "nerian.de") {\
            top.location = "\/support\/dokumentationen\/python-api-doku\/?" + location.href.replace(\/^.*\\\/\\\/[^\\\/]+\\\/\/, ""); \
        } else {\
            top.location = "\/support\/documentation\/python-api-doc\/?" + location.href.replace(\/^.*\\\/\\\/[^\\\/]+\\\/\/, ""); \
        }\
    }\
<\/script>\
<\/head>\
/' visiontransfer.html || exit 1

# Upload
scp visiontransfer.html web45@s177.goserver.host:/home/www/nerian-wordpress/nerian-content/api-doc/python.html

popd
