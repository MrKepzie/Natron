#!/bin/sh
#
# Build and package Natron for Windows
#
# 
# Options (optional):
# NOPKG=1 : Don't build installer/repo
# NOCLEAN=1 : Don't remove sdk installation
# REBUILD_SDK=1 : Trigger rebuild of sdk
# NOBUILD=1 : Don't build anything
# SYNC=1 : Sync files with server
# ONLY_NATRON=1 : Don't build plugins
# ONLY_PLUGINS=1 : Don't build natron
# IO=1 : Enable io plug
# MISC=1 : Enable misc plug
# ARENA=1 : Enable arena plug
# CV=1 : Enable cv plug
# OFFLINE_INSTALLER=1: Build offline installer in addition to the online installer
# SNAPSHOT=1 : Tag build as snapshot
# TARSRC=1 : tar sources
# NATRON_LICENSE=(GPL,COMMERCIAL): When GPL, GPL binaries are bundled with Natron
# USAGE: NATRON_LICENSE=<license> build.sh BIT "branch" noThreads

source `pwd`/common.sh || exit 1

PID=$$
if [ -f $TMP_DIR/natron-build.pid ]; then
    OLDPID=`cat $TMP_DIR/natron-build.pid`
    PIDS=`ps aux|awk '{print $2}'`
    for i in $PIDS;do
        if [ "$i" = "$OLDPID" ]; then
            echo "already running ..."
            exit 1
        fi
    done
fi
echo $PID > $TMP_DIR/natron-build.pid || exit 1

if [ "$OS" = "Msys" ]; then
    PKGOS=Windows
else
    echo "Windows-Msys2-only!"
    exit 1
fi

if [ "$NATRON_LICENSE" != "GPL" -a "$NATRON_LICENSE" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi

if [ "$1" = "32" ]; then
    BIT=32
else
    BIT=64
fi

if [ "$2" = "workshop" ]; then
    BRANCH=$2
    REPO_SUFFIX=snapshot
else
    REPO_SUFFIX=release
fi

if [ "$3" != "" ]; then
    JOBS=$3
else
    #Default to 4 threads
    JOBS=$DEFAULT_MKJOBS
fi

if [ "$NOCLEAN" != "1" ]; then
    rm -rf $INSTALL_PATH
fi
if [ "$REBUILD_SDK" = "1" ]; then
    rm -f $SRC_PATH/Natron*SDK.tar.xz
fi

if [ -z "$IO" ]; then
    IO=1
fi
if [ -z "$MISC" ]; then
    MISC=1
fi
if [ -z "$ARENA" ]; then
    ARENA=1
fi
if [ -z "$CV" ]; then
    CV=1
fi
if [ -z "$OFFLINE_INSTALLER" ]; then
    OFFLINE_INSTALLER=1
fi

REPO_DIR=$REPO_DIR_PREFIX$REPO_SUFFIX

#Make log directory
LOGS=$REPO_DIR/logs
rm -rf $LOGS
if [ ! -d $LOGS ]; then
    mkdir -p $LOGS || exit 1
fi

FAIL=0

if [ ! -f `pwd`/commits-hash.sh ]; then
    touch $CWD/commits-hash.sh
    echo "#!/bin/sh" >> $CWD/commits-hash.sh
    echo "NATRON_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "IOPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "MISCPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "ARENAPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "CVPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "NATRON_VERSION_NUMBER=#" >> $CWD/commits-hash.sh
fi


if [ "$NOBUILD" != "1" ]; then
    if [ "$ONLY_PLUGINS" != "1" ]; then
        echo -n "Building Natron ... "
        env NATRON_LICENSE=$NATRON_LICENSE MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_SNAPSHOT=${SNAPSHOT} sh $INC_PATH/scripts/build-natron.sh $BIT $BRANCH >& $LOGS/natron.$PKGOS$BIT.$TAG.log || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            sleep 2
            cat $LOGS/natron.$PKGOS$BIT.$TAG.log
        fi
    fi
    if [ "$FAIL" != "1" -a "$ONLY_NATRON" != "1" ]; then
        echo -n "Building Plugins ... "
        env NATRON_LICENSE=$NATRON_LICENSE MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_CV=$CV BUILD_IO=$IO BUILD_MISC=$MISC BUILD_ARENA=$ARENA sh $INC_PATH/scripts/build-plugins.sh $BIT $BRANCH >& $LOGS/plugins.$PKGOS$BIT.$TAG.log || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            sleep 2
            cat $LOGS/plugins.$PKGOS$BIT.$TAG.log
        fi  
    fi
fi

if [ "$NOPKG" != "1" -a "$FAIL" != "1" ]; then
    echo -n "Building Packages ... "
    env NATRON_LICENSE=$NATRON_LICENSE OFFLINE=${OFFLINE_INSTALLER} NOTGZ=1 BUNDLE_CV=0 BUNDLE_IO=$IO BUNDLE_MISC=$MISC BUNDLE_ARENA=$ARENA sh $INC_PATH/scripts/build-installer.sh $BIT $BRANCH   >& $LOGS/installer.$PKGOS$BIT.$TAG.log || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat $LOGS/installer.$PKGOS$BIT.$TAG.log
    fi 
fi

if [ "$SYNC" = "1" ]; then
    if [ "$BRANCH" = "workshop" ]; then
        ONLINE_REPO_BRANCH=snapshots
    else
        ONLINE_REPO_BRANCH=releases
    fi
    BIT_SUFFIX=bit
    BIT_TAG=$BIT$BIT_SUFFIX
    if [ "$FAIL" != "1" ]; then
        echo "Syncing packages ... "
        rsync -avz --progress --delete --verbose -e ssh  $REPO_DIR/packages/ $REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/packages
        rsync -avz --progress  --verbose -e ssh $REPO_DIR/installers/ $REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/files
    fi
    rsync -avz --progress --delete --verbose -e ssh $LOGS/ $REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/logs
fi




if [ "$FAIL" = "1" ]; then
    exit 1
else
    exit 0
fi
