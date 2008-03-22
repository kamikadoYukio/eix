#!/bin/sh
# Compare the out of the installed eix with the output of the eix in ../src.
#
# This file is part of the eix project and distributed under the
# terms of the GNU General Public License v2.
#
# Copyright (c)
#   Emil Beinroth <emilbeinroth@gmx.net>

die() {
    echo "!!! failed $@"
    exit 1
}

installed_cache=$(tempfile)
testing_cache=$(tempfile)
installed_output=$(tempfile)
testing_output=$(tempfile)

cleanup_tempfiles() {
    rm -f -- "$installed_output" "$installed_cache" \
             "$testing_output" "$testing_cache"
}

trap cleanup_tempfiles SIGINT SIGHUP SIGTERM SIGKILL

eix_prefix="${0%/*}/../src"

echo testing eix located in "$eix_prefix"

make -C "$eix_prefix"

echo '>> running installed update-eix'
EIX_CACHEFILE="$installed_cache" update-eix || die

echo '>> running installed update-eix'
EIX_CACHEFILE="$testing_cache" "$eix_prefix"/update-eix || did

EIX_CACHEFILE="$installed_cache" eix > "$installed_output" || die
EIX_CACHEFILE="$testing_cache" "$eix_prefix"/eix > "$testing_output" || die


echo "------- diffing -------"

diff "$installed_output" "$testing_output"

cleanup_tempfiles