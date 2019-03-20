#! /bin/bash

# Version number generation script.

# Copyright (C) 2017  Embecosm Limited <info@embecosm.com>
#
# This file is part of the RISC-V GDB server
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Usage:
#   make-version.sh SOURCE-DIRECTORY VERSION-FILENAME
#
# The script tries to run 'git describe' in SOURCE-DIRECTORY, and generates
# a file into VERSION-FILENAME which contains a C #define that can then be
# used within a program.
#
# The name of the generated #define is 'GIT_VERSION'.
#
# If 'git' can't be found then the string "unknown" is used as the git
# version.
#
# The file content is first generated into a temporary file, and then
# moved into place if the new content is different to the old.  This
# should help with Makefiles, as the generated file will only appear
# to change when its content changes.

# Extract command line arguments.
srcdir=$1
version_h=$2

# The temporary filename into which we first generate the content.
tmp_file=${version_h}.$$.tmp

# Get the git description string (if possible).
if which git >/dev/null 2>/dev/null
then
    GIT_FLAGS="--dirty"

    if git describe --help | grep --silent -- --broken
    then
        GIT_FLAGS="$GIT_FLAGS --broken"
    fi

    DESC=$(cd ${srcdir} && git describe $GIT_FLAGS)
else
    DESC="unknown"
fi

# Write out the generated file contents.
echo -n "" > ${tmp_file}
echo "#ifndef __GIT_VERSION_FILE__" >> ${tmp_file}
echo "#define __GIT_VERSION_FILE__" >> ${tmp_file}
echo "#define GIT_VERSION \"${DESC}\"" >> ${tmp_file}
echo "#endif /* __GIT_VERSION_FILE__ */" >> ${tmp_file}

# Move the new contents into place if they are different, make sure we
# get rid of the temporary file either way.
if ! cmp -s ${tmp_file} ${version_h}
then
    mv ${tmp_file} ${version_h}
else
    rm ${tmp_file}
fi
