#!/bin/sh
# Directory Opus 5 translated catalogs
# Copyright 2026 Dimitris Panokostas
#
#   catalogs.sh build <out>          compiles every <language>/<name>.ct here
#                                    with its .cd from source/ into
#                                    <out>/<language>/<catalog>; only
#                                    catalogs older than their .ct or .cd
#                                    are compiled again
#   catalogs.sh stage <out> <stage>  copies them to <stage>/Catalogs/<name>,
#                                    <name> being the "## language" line of
#                                    the .ct files (ISO-8859-1, spelt as
#                                    locale.library spells the language)
#
# The catalog is named after the .cd ("XADopus.cd" -> "XADopus.catalog"),
# except font.cd, which is ViewFont's catalog.  FLEXCAT names the compiler
# (default flexcat).  Stage into a native Linux directory: macOS volumes,
# also through a Docker bind mount, refuse ISO-8859-1 file names.

set -e
LC_ALL=C
export LC_ALL

here=$(cd "$(dirname "$0")" && pwd)
source_dir=$(cd "$here/../source" && pwd)
flexcat=${FLEXCAT:-flexcat}

usage()
{
	echo "usage: $0 build <out> | stage <out> <stage>" >&2
	exit 2
}

# .cd for translation $1 (name without .ct)
descriptor()
{
	found=$(find "$source_dir" -type f -iname "$1.cd")
	case $found in
	"")
		echo "catalogs.sh: no $1.cd under source/" >&2
		exit 1
		;;
	*"
"*)
		echo "catalogs.sh: more than one $1.cd under source/:" >&2
		echo "$found" >&2
		exit 1
		;;
	esac
	echo "$found"
}

catalog_name()
{
	name=$(basename "$1" .cd)
	if [ "$name" = font ]; then
		echo viewfont.catalog
	else
		echo "$name.catalog"
	fi
}

# "## language" of translation file $1
language()
{
	sed -n '/^## language/{s/^## language[[:space:]]*//;s/[[:space:]]*$//;p;q;}' "$1"
}

build()
{
	out=$1
	built=0
	for ct in "$here"/*/*.ct; do
		dir=$(basename "$(dirname "$ct")")
		cd_file=$(descriptor "$(basename "$ct" .ct)")
		catalog="$out/$dir/$(catalog_name "$cd_file")"
		if [ -f "$catalog" ] && [ ! "$ct" -nt "$catalog" ] && [ ! "$cd_file" -nt "$catalog" ]; then
			continue
		fi
		if ! command -v "$flexcat" >/dev/null 2>&1; then
			echo "catalogs.sh: $flexcat not found; build the catalogs where FlexCat is installed (make <platform> catalogs)." >&2
			exit 1
		fi
		mkdir -p "$out/$dir"
		# FlexCat returns 5 for warnings (untranslated strings fall back to
		# the built-in English ones); anything higher is an error.  Write
		# to a temporary name, so a failed run is not taken as up to date.
		rc=0
		"$flexcat" "$cd_file" "$ct" CATALOG "$catalog.tmp" QUIET || rc=$?
		if [ "$rc" -gt 5 ] || [ ! -s "$catalog.tmp" ]; then
			rm -f "$catalog.tmp"
			echo "catalogs.sh: $flexcat failed ($rc) on $ct" >&2
			exit 1
		fi
		mv -f "$catalog.tmp" "$catalog"
		built=$((built + 1))
	done
	echo "Catalogs: $built compiled into $out"
}

stage()
{
	out=$1
	stage_dir=$2
	for dir_path in "$here"/*/; do
		dir=$(basename "$dir_path")
		lang=
		for ct in "$dir_path"*.ct; do
			[ -f "$ct" ] || continue
			ct_lang=$(language "$ct")
			if [ -z "$ct_lang" ]; then
				echo "catalogs.sh: $ct has no ## language line" >&2
				exit 1
			fi
			if [ -z "$lang" ]; then
				lang=$ct_lang
			elif [ "$ct_lang" != "$lang" ]; then
				echo "catalogs.sh: $ct is not \"## language $lang\" like the other $dir files" >&2
				exit 1
			fi
			catalog="$out/$dir/$(catalog_name "$(descriptor "$(basename "$ct" .ct)")")"
			if [ ! -f "$catalog" ]; then
				echo "catalogs.sh: $catalog is missing; run catalogs.sh build first" >&2
				exit 1
			fi
			mkdir -p "$stage_dir/Catalogs/$lang"
			cp -p "$catalog" "$stage_dir/Catalogs/$lang/"
		done
	done
}

case $1 in
build)
	[ $# -eq 2 ] || usage
	build "$2"
	;;
stage)
	[ $# -eq 3 ] || usage
	stage "$2" "$3"
	;;
*)
	usage
	;;
esac
