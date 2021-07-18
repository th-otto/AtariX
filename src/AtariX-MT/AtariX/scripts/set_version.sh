#!/bin/sh

PLIST="$TARGET_BUILD_DIR/$INFOSTRINGS_PATH"
echo "$PLIST"
if test -f "$PLIST"; then
	echo "Setting version in ${INFOSTRINGS_PATH} to ${PROJECT_VERSION}"
	iconv --from UTF-16LE --to UTF-8 $PLIST 2>/dev/null | sed "s/@VERSION@/${PROJECT_VERSION}/" | iconv --from UTF-8 --to UTF-16LE > "${PLIST}.new"
	mv "${PLIST}.new" "${PLIST}"
fi
