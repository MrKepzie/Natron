#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

#
# Build packages and installer for Linux
#

source $(pwd)/common.sh || exit 1
source $(pwd)/commits-hash.sh || exit 1

PID=$$
if [ -f $TMP_DIR/natron-build-installer.pid ]; then
    OLDPID=$(cat $TMP_DIR/natron-build-installer.pid)
    PIDS=$(ps aux|awk '{print $2}')
    for i in $PIDS;do
        if [ "$i" = "$OLDPID" ]; then
            echo "already running ..."
            exit 1
        fi
    done
fi
echo $PID > $TMP_DIR/natron-build-installer.pid || exit 1

if [ "$1" = "workshop" ]; then
    NATRON_VERSION=$NATRON_DEVEL_GIT
    REPO_BRANCH=snapshots
else
    NATRON_VERSION=$NATRON_VERSION_NUMBER
    REPO_BRANCH=releases
fi

DATE=$(date +%Y-%m-%d)
PKGOS=Linux-x86_${BIT}bit
REPO_OS=Linux/$REPO_BRANCH/${BIT}bit/packages

LD_LIBRARY_PATH=$INSTALL_PATH/lib
PATH=$INSTALL_PATH/gcc/bin:$INSTALL_PATH/bin:$PATH

if [ "$ARCH" = "x86_64" ]; then
    LD_LIBRARY_PATH=$INSTALL_PATH/gcc/lib64:$LD_LIBRARY_PATH
else
    LD_LIBRARY_PATH=$INSTALL_PATH/gcc/lib:$LD_LIBRARY_PATH
fi
export LD_LIBRARY_PATH

if [ -d $TMP_PATH ]; then
    rm -rf $TMP_PATH || exit 1
fi
mkdir -p $TMP_PATH || exit 1

# SETUP
INSTALLER=$TMP_PATH/Natron-installer
XML=$INC_PATH/xml
QS=$INC_PATH/qs

mkdir -p $INSTALLER/config $INSTALLER/packages || exit 1
cat $INC_PATH/config/config.xml | sed "s/_VERSION_/${NATRON_VERSION_NUMBER}/;s#_OS_BRANCH_BIT_#${REPO_OS}#g;s#_URL_#${REPO_URL}#g" > $INSTALLER/config/config.xml || exit 1
cp $INC_PATH/config/*.png $INSTALLER/config/ || exit 1

if [ "$NATRON_LICENSE" = "GPL" ]; then
    FFLIC=gpl
else
    FFLIC=lgpl
fi

# OFX IO
OFX_IO_VERSION=$TAG
OFX_IO_PATH=$INSTALLER/packages/$IOPLUG_PKG
mkdir -p $OFX_IO_PATH/data $OFX_IO_PATH/meta $OFX_IO_PATH/data/Plugins $OFX_IO_PATH/data/bin || exit 1
cat $XML/openfx-io.xml | sed "s/_VERSION_/${OFX_IO_VERSION}/;s/_DATE_/${DATE}/" > $OFX_IO_PATH/meta/package.xml || exit 1
cat $QS/openfx-io.qs > $OFX_IO_PATH/meta/installscript.qs || exit 1
cp $INSTALL_PATH/ffmpeg-$FFLIC/bin/{ffmpeg,ffprobe} $OFX_IO_PATH/data/bin/ || exit 1
cat $CWD/include/scripts/ffmpeg.sh > $OFX_IO_PATH/data/ffmpeg || exit 1
cat $CWD/include/scripts/ffmpeg.sh | sed 's/ffmpeg/ffprobe/g' > $OFX_IO_PATH/data/ffprobe || exit 1
chmod +x $OFX_IO_PATH/data/{ffmpeg,ffprobe} || exit 1
cp -a $INSTALL_PATH/Plugins/IO.ofx.bundle $OFX_IO_PATH/data/Plugins/ || exit 1
strip -s $OFX_IO_PATH/data/Plugins/*/*/*/*
IO_LIBS=$OFX_IO_PATH/data/Plugins/IO.ofx.bundle/Libraries
mkdir -p $IO_LIBS || exit 1

OFX_DEPENDS=$(ldd $INSTALLER/packages/*/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_DEPENDS; do
    cp -v $x $IO_LIBS/ || exit 1
done

cp -v $INSTALL_PATH/ffmpeg-$FFLIC/lib/{libavfilter.so.5,libavdevice.so.56,libpostproc.so.53,libavresample.so.2,libavformat.so.56,libavcodec.so.56,libswscale.so.3,libavutil.so.54,libswresample.so.1} $IO_LIBS/ || exit 1
OFX_LIB_DEP=$(ldd $IO_LIBS/*|grep opt | awk '{print $3}')
for y in $OFX_LIB_DEP; do
    cp -v $y $IO_LIBS/ || exit 1
done

rm -f $IO_LIBS/{libgcc*,libstdc*,libbz2*,libfont*,libfree*,libpng*,libjpeg*,libtiff*,libz.*}
(cd $IO_LIBS ;
  ln -sf ../../../lib/libbz2.so.1.0 .
  ln -sf ../../../lib/libbz2.so.1 .
  ln -sf ../../../lib/libfontconfig.so.1 .
  ln -sf ../../../lib/libfreetype.so.6 .
  ln -sf ../../../lib/libpng12.so.0 .
  ln -sf ../../../lib/libjpeg.so.9 .
  ln -sf ../../../lib/libtiff.so.5 .
  ln -sf ../../../lib/libz.so.1 .
  ln -sf ../../../lib/libgcc_s.so.1 .
  ln -sf ../../../lib/libstdc++.so.6 .
)
strip -s $IO_LIBS/*

# OFX MISC
OFX_MISC_VERSION=$TAG
OFX_MISC_PATH=$INSTALLER/packages/$MISCPLUG_PKG
mkdir -p $OFX_MISC_PATH/data $OFX_MISC_PATH/meta $OFX_MISC_PATH/data/Plugins || exit 1
cat $XML/openfx-misc.xml | sed "s/_VERSION_/${OFX_MISC_VERSION}/;s/_DATE_/${DATE}/" > $OFX_MISC_PATH/meta/package.xml || exit 1
cat $QS/openfx-misc.qs > $OFX_MISC_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/Plugins/{CImg,Misc}.ofx.bundle $OFX_MISC_PATH/data/Plugins/ || exit 1
strip -s $OFX_MISC_PATH/data/Plugins/*/*/*/*
CIMG_LIBS=$OFX_MISC_PATH/data/Plugins/CImg.ofx.bundle/Libraries
mkdir -p $CIMG_LIBS || exit 1

OFX_CIMG_DEPENDS=$(ldd $OFX_MISC_PATH/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_CIMG_DEPENDS; do
    cp -v $x $CIMG_LIBS/ || exit 1
done
rm -f $CIMG_LIBS/{libgcc*,libstdc*,libgomp*}
(cd $CIMG_LIBS ;
  ln -sf ../../../lib/libgcc_s.so.1 .
  ln -sf ../../../lib/libstdc++.so.6 .
  ln -sf ../../../lib/libgomp.so.1 .
)
strip -s $CIMG_LIBS/*
MISC_LIBS=$OFX_MISC_PATH/data/Plugins/Misc.ofx.bundle/Libraries
mkdir -p $MISC_LIBS || exit 1
(cd $MISC_LIBS ;
  ln -sf ../../../lib/libgcc_s.so.1 .
  ln -sf ../../../lib/libstdc++.so.6 .
)

# NATRON
NATRON_PATH=$INSTALLER/packages/$NATRON_PKG
mkdir -p $NATRON_PATH/meta $NATRON_PATH/data/docs $NATRON_PATH/data/bin $NATRON_PATH/data/share || exit 1
cat $XML/natron.xml | sed "s/_VERSION_/${TAG}/;s/_DATE_/${DATE}/" > $NATRON_PATH/meta/package.xml || exit 1
cat $QS/natron.qs > $NATRON_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/docs/natron/* $NATRON_PATH/data/docs/ || exit 1
cp $INSTALL_PATH/share/stylesheets/mainstyle.qss $NATRON_PATH/data/share/ || exit 1
cat $INSTALL_PATH/docs/natron/LICENSE.txt > $NATRON_PATH/meta/natron-license.txt || exit 1
cp $INSTALL_PATH/bin/Natron* $NATRON_PATH/data/bin/ || exit 1
strip -s $NATRON_PATH/data/bin/Natron $NATRON_PATH/data/bin/NatronRenderer
if [ -f "$NATRON_PATH/data/bin/NatronCrashReporter" ]; then
    strip -s $NATRON_PATH/data/bin/NatronCrashReporter $NATRON_PATH/data/bin/NatronRendererCrashReporter
fi

wget $NATRON_API_DOC || exit 1
mv natron.pdf $NATRON_PATH/data/docs/Natron_Python_API_Reference.pdf || exit 1
rm $NATRON_PATH/data/docs/TuttleOFX-README.txt || exit 1
cat $INC_PATH/scripts/Natron.sh > $NATRON_PATH/data/Natron || exit 1
cat $INC_PATH/scripts/Natron.sh | sed "s#bin/Natron#bin/NatronRenderer#" > $NATRON_PATH/data/NatronRenderer || exit 1
chmod +x $NATRON_PATH/data/Natron $NATRON_PATH/data/NatronRenderer || exit 1

# OCIO
OCIO_VERSION=$COLOR_PROFILES_VERSION
OCIO_PATH=$INSTALLER/packages/$PROFILES_PKG
mkdir -p $OCIO_PATH/meta $OCIO_PATH/data/share || exit 1
cat $XML/ocio.xml | sed "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${DATE}/" > $OCIO_PATH/meta/package.xml || exit 1
cat $QS/ocio.qs > $OCIO_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/share/OpenColorIO-Configs $OCIO_PATH/data/share/ || exit 1

# CORE LIBS
CLIBS_VERSION=$CORELIBS_VERSION
CLIBS_PATH=$INSTALLER/packages/$CORELIBS_PKG
mkdir -p $CLIBS_PATH/meta $CLIBS_PATH/data/bin $CLIBS_PATH/data/lib $CLIBS_PATH/data/share/pixmaps || exit 1
cat $XML/corelibs.xml | sed "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${DATE}/" > $CLIBS_PATH/meta/package.xml || exit 1
cat $QS/corelibs.qs > $CLIBS_PATH/meta/installscript.qs || exit 1
cp $INSTALL_PATH/lib/libQtDBus.so.4 $CLIBS_PATH/data/lib/ || exit 1
cp $INSTALL_PATH/share/pixmaps/natronIcon256_linux.png $CLIBS_PATH/data/share/pixmaps/ || exit 1

cp -a $INSTALL_PATH/plugins/* $CLIBS_PATH/data/bin/ || exit 1
CORE_DEPENDS=$(ldd $NATRON_PATH/data/bin/*|grep opt | awk '{print $3}')
for i in $CORE_DEPENDS; do
    cp -v $i $CLIBS_PATH/data/lib/ || exit 1
done
LIB_DEPENDS=$(ldd $CLIBS_PATH/data/lib/*|grep opt | awk '{print $3}')
for y in $LIB_DEPENDS; do
    cp -v $y $CLIBS_PATH/data/lib/ || exit 1
done
PLUG_DEPENDS=$(ldd $CLIBS_PATH/data/bin/*/*|grep opt | awk '{print $3}')
for z in $PLUG_DEPENDS; do
    cp -v $z $CLIBS_PATH/data/lib/ || exit 1
done
if [ -f $INC_PATH/misc/compat${BIT}.tgz ]; then
    tar xvf $INC_PATH/misc/compat${BIT}.tgz -C $CLIBS_PATH/data/lib/ || exit 1
fi

cp $INSTALL_PATH/lib/{libicudata.so.55,libicui18n.so.55,libicuuc.so.55} $CLIBS_PATH/data/lib/ || exit 1
(cd $CLIBS_PATH/data/lib;
  rm -f libbz2.so.1.0
  ln -sf libbz2.so.1 libbz2.so.1.0
)

mv $IO_LIBS/{libOpenColor*,libgomp*} $CLIBS_PATH/data/lib/ || exit 1
(cd $IO_LIBS ;
  ln -sf ../../../lib/libOpenColorIO.so.1 .
  ln -sf ../../../lib/libgomp.so.1 .
)

mkdir -p $CLIBS_PATH/data/share/etc/fonts/conf.d || exit 1
cp $INSTALL_PATH/etc/fonts/fonts.conf $CLIBS_PATH/data/share/etc/fonts/ || exit 1
cp $INSTALL_PATH/share/fontconfig/conf.avail/* $CLIBS_PATH/data/share/etc/fonts/conf.d/ || exit 1
sed -i "s#${SDK_PATH}/Natron-${SDK_VERSION}/#/#;/conf.d/d" $CLIBS_PATH/data/share/etc/fonts/fonts.conf || exit 1
(cd $CLIBS_PATH/data ; ln -sf share Resources )

# TODO: At this point send unstripped binaries (and debug binaries?) to Socorro server for breakpad

strip -s $CLIBS_PATH/data/lib/*
strip -s $CLIBS_PATH/data/bin/*/*

#Copy Python distrib
mkdir -p $CLIBS_PATH/data/Plugins || exit 1
if [ "$PYV" = "3" ]; then
    cp -a $INSTALL_PATH/lib/python3.4 $CLIBS_PATH/data/lib/ || exit 1
    mv $CLIBS_PATH/data/lib/python3.4/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
    (cd $CLIBS_PATH/data/lib/python3.4/site-packages; ln -sf ../../../Plugins/PySide . )
    rm -rf $CLIBS_PATH/data/lib/python3.4/{test,config-3.4m} || exit 1
else
    cp -a $INSTALL_PATH/lib/python2.7 $CLIBS_PATH/data/lib/ || exit 1
    mv $CLIBS_PATH/data/lib/python2.7/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
    (cd $CLIBS_PATH/data/lib/python2.7/site-packages; ln -sf ../../../Plugins/PySide . )
    rm -rf $CLIBS_PATH/data/lib/python2.7/{test,config} || exit 1
fi
PY_DEPENDS=$(ldd $CLIBS_PATH/data/Plugins/PySide/*|grep opt | awk '{print $3}')
for y in $PY_DEPENDS; do
    cp -v $y $CLIBS_PATH/data/lib/ || exit 1
done
(cd $CLIBS_PATH ; find . -type d -name __pycache__ -exec rm -rf {} \;)
strip -s $CLIBS_PATH/data/Plugins/PySide/* $CLIBS_PATH/data/lib/python*/* $CLIBS_PATH/data/lib/python*/*/*

# OFX ARENA
OFX_ARENA_VERSION=$TAG
OFX_ARENA_PATH=$INSTALLER/packages/$ARENAPLUG_PKG
mkdir -p $OFX_ARENA_PATH/meta $OFX_ARENA_PATH/data/Plugins || exit 1
cat $XML/openfx-arena.xml | sed "s/_VERSION_/${OFX_ARENA_VERSION}/;s/_DATE_/${DATE}/" > $OFX_ARENA_PATH/meta/package.xml || exit 1
cat $QS/openfx-arena.qs > $OFX_ARENA_PATH/meta/installscript.qs || exit 1
cp -av $INSTALL_PATH/Plugins/Arena.ofx.bundle $OFX_ARENA_PATH/data/Plugins/ || exit 1
strip -s $OFX_ARENA_PATH/data/Plugins/*/*/*/*

ARENA_LIBS=$OFX_ARENA_PATH/data/Plugins/Arena.ofx.bundle/Libraries
mkdir -p $ARENA_LIBS || exit 1
OFX_ARENA_DEPENDS=$(ldd $OFX_ARENA_PATH/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_ARENA_DEPENDS; do
    cp -v $x $ARENA_LIBS/ || exit 1
done
rm -f $ARENA_LIBS/{libgomp*,libOpenColorIO*,libbz2*,libfont*,libz.so*,libglib-2*,libgthread*,libpng*,libfree*,libexpat*,libgcc*,libstdc*}
(cd $ARENA_LIBS ; 
  ln -sf ../../../lib/libbz2.so.1.0 .
  ln -sf ../../../lib/libbz2.so.1 .
  ln -sf ../../../lib/libexpat.so.1 .
  ln -sf ../../../lib/libfontconfig.so.1 .
  ln -sf ../../../lib/libfreetype.so.6 .
  ln -sf ../../../lib/libglib-2.0.so.0 .
  ln -sf ../../../lib/libgthread-2.0.so.0 .
  ln -sf ../../../lib/libpng12.so.0 .
  ln -sf ../../../lib/libz.so.1 .
  ln -sf ../../../lib/libgcc_s.so.1 .
  ln -sf ../../../lib/libstdc++.so.6 .
  ln -sf ../../../lib/libOpenColorIO.so.1 .
  ln -sf ../../../lib/libgomp.so.1 .
)
strip -s $ARENA_LIBS/*

# OFX CV
#OFX_CV_VERSION=$TAG
#OFX_CV_PATH=$INSTALLER/packages/$CVPLUG_PKG
#mkdir -p $OFX_CV_PATH/{data,meta} $OFX_CV_PATH/data/Plugins $OFX_CV_PATH/data/docs/openfx-opencv || exit 1
#cat $XML/openfx-opencv.xml | sed "s/_VERSION_/${OFX_CV_VERSION}/;s/_DATE_/${DATE}/" > $OFX_CV_PATH/meta/package.xml || exit 1
#cat $QS/openfx-opencv.qs > $OFX_CV_PATH/meta/installscript.qs || exit 1
#cp -a $INSTALL_PATH/docs/openfx-opencv $OFX_CV_PATH/data/docs/ || exit 1
#cat $OFX_CV_PATH/data/docs/openfx-opencv/README > $OFX_CV_PATH/meta/license.txt || exit 1
#cp -a $INSTALL_PATH/Plugins/{inpaint,segment}.ofx.bundle $OFX_CV_PATH/data/Plugins/ || exit 1
#strip -s $OFX_CV_PATH/data/Plugins/*/*/*/*

#mkdir -p $OFX_CV_PATH/data/lib || exit 1
#OFX_CV_DEPENDS=$(ldd $OFX_CV_PATH/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
#for x in $OFX_CV_DEPENDS; do
#  cp -v $x $OFX_CV_PATH/data/lib/ || exit 1
#done
#strip -s $OFX_CV_PATH/data/lib/*
#cp -a $INSTALL_PATH/docs/opencv $OFX_CV_PATH/data/docs/ || exit 1
#cat $INSTALL_PATH/docs/opencv/LICENSE >> $OFX_CV_PATH/meta/ofx-cv-license.txt || exit 1

#mkdir -p $OFX_CV_PATH/data/Plugins/inpaint.ofx.bundle/Libraries || exit 1
#mv $OFX_CV_PATH/data/lib/* $OFX_CV_PATH/data/Plugins/inpaint.ofx.bundle/Libraries/ || exit 1
#(cd $OFX_CV_PATH/data/Plugins/segment.ofx.bundle; ln -sf ../inpaint.ofx.bundle/Libraries .)
#rm -rf $OFX_CV_PATH/data/lib || exit 1

# Clean and perms
chown root:root -R $INSTALLER/*
(cd $INSTALLER; find . -type d -name .git -exec rm -rf {} \;)

# Build repo and package
if [ "$NO_INSTALLER" != "1" ]; then
    if [ "$1" = "workshop" ]; then
        ONLINE_TAG=snapshot
    else
        ONLINE_TAG=release
    fi

    ONLINE_INSTALL=Natron-${PKGOS}-online-install-$ONLINE_TAG
    BUNDLED_INSTALL=Natron-$NATRON_VERSION-${PKGOS}

    REPO_DIR=$REPO_DIR_PREFIX$ONLINE_TAG
    rm -rf $REPO_DIR/packages $REPO_DIR/installers

    mkdir -p $REPO_DIR/packages || exit 1

    $INSTALL_PATH/bin/repogen -v --update-new-components -p $INSTALLER/packages -c $INSTALLER/config/config.xml $REPO_DIR/packages || exit 1

    mkdir -p $REPO_DIR/installers || exit 1

    if [ "$OFFLINE" != "0" ]; then
        $INSTALL_PATH/bin/binarycreator -v -f -p $INSTALLER/packages -c $INSTALLER/config/config.xml -i $PACKAGES $REPO_DIR/installers/$BUNDLED_INSTALL || exit 1 
        cd $REPO_DIR/installers || exit 1
        tar cvvzf $BUNDLED_INSTALL.tgz $BUNDLED_INSTALL || exit 1
        ln -sf $BUNDLED_INSTALL.tgz Natron-latest-$PKGOS-$ONLINE_TAG.tgz || exit 1
    fi

    $INSTALL_PATH/bin/binarycreator -v -n -p $INSTALLER/packages -c $INSTALLER/config/config.xml $ONLINE_INSTALL || exit 1
    tar cvvzf $ONLINE_INSTALL.tgz $ONLINE_INSTALL || exit 1
fi

rm $REPO_DIR/installers/$ONLINE_INSTALL $REPO_DIR/installers/$BUNDLED_INSTALL

echo "All Done!!!"
