#!/bin/sh

#
# actual build script
#
export BUILDROOT="${PWD}/.travis/tmp"
export OUT="${PWD}/.travis/out"

isrelease=false
export isrelease

VERSION=`date +%Y%m%d-%H%M%S`
ATAG=${VERSION}
BINARCHIVE="${PROJECT_LOWER}-${ATAG}-bin.zip"
SRCARCHIVE="${PROJECT_LOWER}-${ATAG}-src.zip"

export BINARCHIVE
export SRCARCHIVE


mkdir -p "${BUILDROOT}"
mkdir -p "${OUT}"

unset CC CXX

prefix=/usr
bindir=$prefix/bin
datadir=$prefix/share
icondir=$datadir/icons/hicolor

case "$TRAVIS_OS_NAME" in
linux)
	;;

osx)
#	DMG="${PROJECT_LOWER}-${VERSION}${archive_tag}.dmg"
	BINARCHIVE="${PROJECT_LOWER}-${ATAG}.dmg"
	BINARCHIVE="${PROJECT}-${ATAG}.app.zip"

	cwd="$PWD"

#	cd "$cwd/src/AtariX-MT/SDL2-2.0.3/Xcode/SDL"
#	xcodebuild -derivedDataPath "$OUT" -project SDL.xcodeproj -configuration Release -scheme Framework -showBuildSettings
#	xcodebuild -derivedDataPath "$OUT" -project SDL.xcodeproj -configuration Release -scheme Framework -quiet || exit 1

	cd "$cwd/src/AtariX-MT/AtariX"
	xcodebuild -derivedDataPath "$OUT" -project AtariX.xcodeproj -configuration Release -scheme AtariX-Application -showBuildSettings
	xcodebuild -derivedDataPath "$OUT" -project AtariX.xcodeproj -configuration Release -scheme AtariX-Application -quiet || exit 1

	cd "${OUT}/Build/Products/Release/" || exit 1
	zip -r "${OUT}/${BINARCHIVE}" AtariX.app || exit 1
	cd "$cwd"
#	mv "$OUT/Build/Products/Release/$DMG" "$OUT/$BINARCHIVE" || exit 1
	;;

esac




git archive --format=zip --prefix=${PROJECT_LOWER}/ HEAD > "${OUT}/${SRCARCHIVE}"
ls -l "${OUT}"
