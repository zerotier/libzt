#!/bin/bash

PROJNAME="zt"
LIBNAME="lib"$PROJNAME
LIBZT_VERSION="1.2.0"
LIBZT_REVISION="1"
ZT_CORE_VERSION="1.2.12"
FILENAME_PREFIX=${LIBNAME}

STAGING_DIR=$(pwd)/staging
STAGING_DEBUG_DIR=$(pwd)/staging/debug
STAGING_RELEASE_DIR=$(pwd)/staging/release
FINISHED_PRODUCTS_DIR=$(pwd)/products

# Clean before zipping
find . -type f \( -name '*.DS_Store' -o -name 'thumbs.db' \) -delete

# Emit a README file
echo $'* libzt version: '${LIBZT_VERSION}$'r'${LIBZT_REVISION}$'\n* Core ZeroTier version: '${ZT_CORE_VERSION}$'\n* Date: '$(date)$'\n\nZeroTier Manual: https://www.zerotier.com/manual.shtml\n
Other Downloads: https://www.zerotier.com/download.shtml
\nlibzt Repo: https://github.com/zerotier/libzt' > ${STAGING_DIR}/README.md

cp ${STAGING_DIR}/README.md ${STAGING_DIR}/debug/README.md
cp ${STAGING_DIR}/README.md ${STAGING_DIR}/release/README.md

# Package everything together
# (debug)
PRODUCT_FILENAME=${FILENAME_PREFIX}-debug.tar.gz
echo "Making: " ${FINISHED_PRODUCTS_DIR}/${PRODUCT_FILENAME}
cd ${STAGING_DEBUG_DIR}
tar --exclude=${PRODUCT_FILENAME} -zcvf ${PRODUCT_FILENAME} .
md5 $PRODUCT_FILENAME
mv *.tar.gz ${FINISHED_PRODUCTS_DIR}
cd -

# (release)
PRODUCT_FILENAME=${FILENAME_PREFIX}-release.tar.gz
echo "Making: " ${FINISHED_PRODUCTS_DIR}/${PRODUCT_FILENAME}
cd ${STAGING_RELEASE_DIR}
tar --exclude=${PRODUCT_FILENAME} -zcvf ${PRODUCT_FILENAME} .
md5 $PRODUCT_FILENAME
mv *.tar.gz ${FINISHED_PRODUCTS_DIR}
cd -