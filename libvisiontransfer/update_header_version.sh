#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "Usage $0 x.x.x"
    exit 1
fi

pushd `dirname $0`/visiontransfer

major=`echo $1 | awk -F '.' '{print $1}'`
minor=`echo $1 | awk -F '.' '{print $2}'`
patch=`echo $1 | awk -F '.' '{print $3}'`

sed -ie "s/VISIONTRANSFER_MAJOR_VERSION.*/VISIONTRANSFER_MAJOR_VERSION\t$major/" common.h
sed -ie "s/VISIONTRANSFER_MINOR_VERSION.*/VISIONTRANSFER_MINOR_VERSION\t$minor/" common.h
sed -ie "s/VISIONTRANSFER_PATCH_VERSION.*/VISIONTRANSFER_PATCH_VERSION\t$patch/" common.h

popd
