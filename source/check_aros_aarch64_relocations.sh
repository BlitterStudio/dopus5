#!/bin/sh
# Reject AROS aarch64 binaries that use a loader-incompatible code model.

set -eu

READELF=${READELF:-aarch64-aros-readelf}
required_relocations='R_AARCH64_MOVW_UABS_G0_NC
R_AARCH64_MOVW_UABS_G1_NC
R_AARCH64_MOVW_UABS_G2_NC
R_AARCH64_MOVW_UABS_G3'
rejected_relocations='R_AARCH64_ADR_GOT_PAGE
R_AARCH64_LD64_GOT_LO12_NC'
failed=0

if [ "$#" -eq 0 ]; then
	echo "usage: $0 <aarch64-aros ELF> [...]" >&2
	exit 2
fi

for binary do
	if ! relocations=$($READELF -rW "$binary"); then
		echo "$binary: unable to inspect relocations" >&2
		failed=1
		continue
	fi

	missing=
	for relocation in $required_relocations; do
		case "$relocations" in
			*"$relocation"*) ;;
			*) missing="$missing $relocation" ;;
		esac
	done

	rejected=
	for relocation in $rejected_relocations; do
		case "$relocations" in
			*"$relocation"*) rejected="$rejected $relocation" ;;
		esac
	done

	if [ -n "$missing" ] || [ -n "$rejected" ]; then
		if [ -n "$missing" ]; then
			echo "$binary: missing loader-compatible absolute relocations:$missing" >&2
		fi
		if [ -n "$rejected" ]; then
			echo "$binary: contains loader-rejected GOT relocations:$rejected" >&2
		fi
		failed=1
	else
		echo "$binary: AROS aarch64 absolute relocations verified"
	fi
done

exit "$failed"
