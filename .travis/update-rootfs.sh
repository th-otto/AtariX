#!/bin/sh

#
# The AtariX emulator and the magicmac applications for atari
# are in separate git repositories.
# This script can be used to populate the rootfs of AtariX,
# which is used to do a fresh install, with the latest build
# files of the magicmac repository.
#

file="$1"

URL="https://dl.bintray.com/th-otto/magicmac-files/snapshots"

SRC_DIR=`pwd`
TMP_DIR=~/atarix.tmp

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
cd "$TMP_DIR"

if test "$file" = ""; then
	wget "$URL/" -O index.html
	file=`sed -n 's/.*href=":\([^"]*-bin[^"]*\)".*/\1/p' index.html | tail -1`
	if test "$file" = ""; then
		echo "$0: no archives found" >&2
		exit 1
	fi
fi

wget "https://dl.bintray.com/th-otto/magicmac-files/snapshots/$file" || exit 1

unzip -q "$file"

cd "$SRC_DIR/src/AtariX-MT/Atarix" || exit 1

function upper()
{
	echo "$@" | tr '[a-z]' '[A-Z]'
}

LANGUAGES="de fr en"
lproj_de=de.lproj
lproj_en=English.lproj
lproj_fr=fr.lproj

set -e

#
# localizations
#
for lang in $LANGUAGES;
do
	eval lproj=\${lproj_$lang}
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/applicat.inf" "$lproj/rootfs/GEMSYS/GEMDESK/APPLICAT.INF"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/applicat.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/APPLICAT.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/chgres.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/CHGRES.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/magxdesk.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MAGXDESK.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mcmd.tos" "$lproj/rootfs/GEMSYS/GEMDESK/MCMD.TOS"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgcopy.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MGCOPY.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgedit.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MGEDIT.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgformat.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MGFORMAT.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgnotice.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MGNOTICE.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgnotice.txt" "$lproj/rootfs/GEMSYS/GEMDESK/MGNOTICE.TXT"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mgsearch.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/MGSEARCH.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/mod_app.txt" "$lproj/rootfs/GEMSYS/GEMDESK/MOD_APP.TXT"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/shutdown.prg" "$lproj/rootfs/GEMSYS/GEMDESK/SHUTDOWN.PRG"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/vfatconf.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/VFATCONF.RSC"
	mv "$TMP_DIR/$lang/GEMSYS/GEMDESK/vt52.rsc" "$lproj/rootfs/GEMSYS/GEMDESK/VT52.RSC"

	mv "$TMP_DIR/$lang/GEMSYS/MAGIC/XTENSION/pdlg.slb" "$lproj/rootfs/GEMSYS/MAGIC/XTENSION/PDLG.SLB"

	mv "$TMP_DIR/$lang/magcmacx.os" "$lproj/MagicMacX.OS"

	mv "$TMP_DIR/$lang/AUTO/ACCS/COPS.ACC" "$lproj/rootfs/AUTO/ACCS/COPS.ACC"

	for f in colorvdi config ttsound general;
	do
		mv "$TMP_DIR/$lang/AUTO/ACCS/CPX/$f.cpx" "$lproj/rootfs/AUTO/ACCS/CPX/$(upper $f).CPX"
	done
	for f in modem printer;
	do
		mv "$TMP_DIR/$lang/AUTO/ACCS/CPX/$f.cpx" "$lproj/rootfs/AUTO/ACCS/CPZ/$(upper $f).CPZ"
	done
done

#
# Common files
#
mv "$TMP_DIR/de/GEMSYS/HOME/cops.inf" "rootfs-common/GEMSYS/HOME/COPS.INF"
rm "$TMP_DIR/en/GEMSYS/HOME/cops.inf"
rm "$TMP_DIR/fr/GEMSYS/HOME/cops.inf"

for f in "$TMP_DIR/de/AUTO/ACCS/"*.PAL;
do
	mv "$f" "rootfs-common/AUTO/ACCS/$(upper $(basename $f))"
done
for f in "$TMP_DIR/en/AUTO/ACCS/"*.PAL "$TMP_DIR/fr/AUTO/ACCS/"*.PAL;
do
	rm "$f"
done

for f in MAGXCONF.CPX TSLICE.CPX clock.CPX ncache.CPX nprnconf.CPX si.CPX;
do
	mv "$TMP_DIR/de/AUTO/ACCS/CPX/$f" "rootfs-common/AUTO/ACCS/CPX/$(upper $(basename $f))"
	rm "$TMP_DIR/en/AUTO/ACCS/CPX/$f"
	rm "$TMP_DIR/fr/AUTO/ACCS/CPX/$f"
done

for f in hddrconf.CPZ maus.CPZ ps.CPZ set_flg.CPZ;
do
	mv "$TMP_DIR/de/AUTO/ACCS/CPZ/$f" "rootfs-common/AUTO/ACCS/CPZ/$(upper $(basename $f))"
	rm "$TMP_DIR/en/AUTO/ACCS/CPZ/$f"
	rm "$TMP_DIR/fr/AUTO/ACCS/CPZ/$f"
done

for f in crashdmp.tos fc.ttp limitmem.ttp memexamn.ttp;
do
	mv "$TMP_DIR/de/BIN/$f" "rootfs-common/BIN/$(upper $f)"
	rm "$TMP_DIR/en/BIN/$f"
	rm "$TMP_DIR/fr/BIN/$f"
done

for f in "$TMP_DIR/de/GEMSYS/GEMDESK/RSC/"*;
do
	mv "$f" "rootfs-common/GEMSYS/GEMDESK/RSC/$(upper $(basename $f))"
done
rmdir "$TMP_DIR/de/GEMSYS/GEMDESK/RSC"
rm -rf "$TMP_DIR/en/GEMSYS/GEMDESK/RSC"
rm -rf "$TMP_DIR/fr/GEMSYS/GEMDESK/RSC"

for f in "$TMP_DIR/de/GEMSYS/GEMDESK/PAT/16/"*;
do
	mv "$f" "rootfs-common/GEMSYS/GEMDESK/PAT/16/$(upper $(basename $f))"
done
for f in "$TMP_DIR/de/GEMSYS/GEMDESK/PAT/256/"*;
do
	mv "$f" "rootfs-common/GEMSYS/GEMDESK/PAT/256/$(upper $(basename $f))"
done
rm -rf "$TMP_DIR/de/GEMSYS/GEMDESK/PAT"
rm -rf "$TMP_DIR/en/GEMSYS/GEMDESK/PAT"
rm -rf "$TMP_DIR/fr/GEMSYS/GEMDESK/PAT"

for f in applicat.app chgres.prg magxdesk.app mgclock.prg mgcopy.app mgedit.app mgformat.prg mgnotice.app \
	mgsearch.app mgview.app mgxclock.prg mod_app.ttp showfile.ttp showfree.tos shutdown.inf vfatconf.prg \
	vt52.prg wbdaemon.prg;
do
	mv "$TMP_DIR/de/GEMSYS/GEMDESK/$f" "rootfs-common/GEMSYS/GEMDESK/$(upper $(basename $f))"
	rm "$TMP_DIR/en/GEMSYS/GEMDESK/$f"
	rm "$TMP_DIR/fr/GEMSYS/GEMDESK/$f"
done


for f in mmxdaemn.prx;
do
	mv "$TMP_DIR/de/GEMSYS/MAGIC/START/$f" "rootfs-common/GEMSYS/MAGIC/START/MMXDAEMN.PRG"
	rm "$TMP_DIR/en/GEMSYS/MAGIC/START/$f"
	rm "$TMP_DIR/fr/GEMSYS/MAGIC/START/$f"
done
for f in dev_ser.dev RAMDISK.XFX SPINMAGC.XFX editobjc.slb load_img.slb winframe.slb winframe.rsc winframe.inf;
do
	mv "$TMP_DIR/de/GEMSYS/MAGIC/XTENSION/$f" "rootfs-common/GEMSYS/MAGIC/XTENSION/$(upper $f)"
	rm "$TMP_DIR/en/GEMSYS/MAGIC/XTENSION/$f"
	rm "$TMP_DIR/fr/GEMSYS/MAGIC/XTENSION/$f"
done
for f in "$TMP_DIR/de/GEMSYS/MAGIC/XTENSION/THEMES/aqua/"*;
do
	mv "$f" "rootfs-common/GEMSYS/MAGIC/XTENSION/THEMES/AQUA/$(upper $(basename $f))"
done
rmdir "$TMP_DIR/de/GEMSYS/MAGIC/XTENSION/THEMES/aqua"
rm -rf "$TMP_DIR/en/GEMSYS/MAGIC/XTENSION/THEMES/aqua"
rm -rf "$TMP_DIR/fr/GEMSYS/MAGIC/XTENSION/THEMES/aqua"

for f in BIN/adr.prg BIN/il0008.prg BIN/il4afc.prg BIN/ilf000.prg BIN/priv_rte.prg BIN/priv_sr.prg BIN/trap7.prg \
	addmem.prg magxboot.prg magxbo32.prg ct60.txt ct60boot.prg hardcopy.prg magic_p.tos magx.inf romdrvr.prg \
	clock/clock.app clock/clock.gen clock/clock.inf clock/clock.man clock/clock.mup clock/clockcol.cpx clock/maus.ruf clock/readme.cat \
	aes_lupe/aes_lupe.app aes_lupe/aes_lupe.img aes_lupe/aes_lupe.txt \
	flock_ok/flock_ok.prg flock_ok/flock_ok.eng flock_ok/flock_ok.txt;
do
	mv "$TMP_DIR/de/EXTRAS/$f" "rootfs-common/EXTRAS/$(upper $f)"
	rm "$TMP_DIR/en/EXTRAS/$f"
	rm "$TMP_DIR/fr/EXTRAS/$f"
done

for f in "$TMP_DIR/de/GEMSYS/"*.SYS "$TMP_DIR/de/GEMSYS/"*.OSD;
do
	mv "$f" "rootfs-common/GEMSYS/$(upper $(basename $f))"
	rm "$TMP_DIR/en/GEMSYS/$(basename $f)"
	rm "$TMP_DIR/fr/GEMSYS/$(basename $f)"
done

for lang in $LANGUAGES; do
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/START
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/STOP
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/UTILITY
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/XTENSION/HIDE
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/XTENSION/HIDE2
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/XTENSION/THEMES
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC/XTENSION
	rmdir $TMP_DIR/$lang/GEMSYS/MAGIC
 	rmdir $TMP_DIR/$lang/GEMSYS/GEMDESK
	rmdir $TMP_DIR/$lang/GEMSYS/HOME
	rmdir $TMP_DIR/$lang/GEMSYS/GEMSCRAP
	rmdir $TMP_DIR/$lang/GEMSYS
	rmdir $TMP_DIR/$lang/BIN
	rmdir $TMP_DIR/$lang/AUTO/ACCS/CPX
	rmdir $TMP_DIR/$lang/AUTO/ACCS/CPZ
	rmdir $TMP_DIR/$lang/AUTO/ACCS/
	rm -f $TMP_DIR/$lang/AUTO/magxbo*
	rm -f $TMP_DIR/$lang/AUTO/autoexec*
	rmdir $TMP_DIR/$lang/AUTO/
	rmdir $TMP_DIR/$lang/EXTRAS/CLOCK
	rmdir $TMP_DIR/$lang/EXTRAS/BIN
	rmdir $TMP_DIR/$lang/EXTRAS/FLOCK_OK
	rmdir $TMP_DIR/$lang/EXTRAS/AES_LUPE
	rmdir $TMP_DIR/$lang/EXTRAS/
	rm -f $TMP_DIR/$lang/autoexec*
	rm -f $TMP_DIR/$lang/*.ram
	rmdir $TMP_DIR/$lang
done
