#!/bin/sh

PLIST="$TARGET_BUILD_DIR/$INFOSTRINGS_PATH"
echo "$PLIST"
if test -f "$PLIST"; then
	echo "Setting version in ${INFOSTRINGS_PATH} to ${PROJECT_VERSION}"
	cat $PLIST | sed "s/@VERSION@/${PROJECT_VERSION}/" > "${PLIST}.new"
	mv "${PLIST}.new" "${PLIST}"
fi
