#!/bin/bash
# =========================================================
# Copyright 2012-2017,  Nuno A. Fonseca (nuno dot fonseca at gmail dot com)
#
#
# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# if not, see <http://www.gnu.org/licenses/>.
#
#
# =========================================================
# wrapper to fastq_info to support BAM and bzip2

FILES=$1
PE_PARAMETER=
if [ "$2-" == "pe-" ]; then
    PE_PARAMETER=pe
else
    FILES=$*
fi

if [ "$1-" == "-" ]; then
    echo "ERROR: fastq_validator.sh file1 [file2|pe]" 
    exit 1
fi

function file_type {
    ## first check if is a bam file
    samtools view -H $1 &> /dev/null
    if [ $? -eq 0 ]; then
	echo "bam"
    else
	## 
	x=$(file -b -i  $1|cut -f 1 -d\;|sed "s|.*/||")
	if [ "$x" == "x-gzip" ]; then echo "gz";
	else
	    if [ "$x" == "x-bzip2" ]; then echo "bzip2";
	    else
		if [ "$x" == "plain" ]; then echo "fastq";
		else
		    echo "Unsupported file type $x" > /dev/stderr
		    exit 4
		fi
	    fi
	fi	
    fi
}

function file_extension {
    filename=$(basename $*)
    ext="${filename##*.}"
    if [ "$ext-" != "$filename-" ]; then
	echo $ext
    fi
}

# check extension
ext=`file_extension $1`

if [ "-$ext" == "-" ]; then
    ext=$(file_type $1)
    echo "File does not have an extension, assuming that it is '.$ext'"
fi

# Check integrity of gzip files
if [ "$ext-" == "gz-" ]; then
    for f in $FILES; do
	echo -n "Checking integrity of gzip file $f..."
	gzip -t $f
	if [ $? -eq 0 ]; then
	    echo "done."
	else
	    echo ""
	    echo "ERROR: Error in file $f: corrupted gzip file"
	    exit 1
	fi
	    
    done
    echo ""
fi

set -eT
#x
if [ "$ext-" == "bam-" ]; then
    #hmm, this now validates bams...kind of
    # samtools version should be 1 or above
    f=$1    
    echo "BAM file"
    # check if the BAM contains unaligned reads
    echo "Checking for unmapped reads"
    UN=`samtools view -c -F 4 $f`
    if [ "$UN-" != "0-" ]; then
	echo "ERROR: Aligned reads found in $f." > /dev/stderr
	exit 1
    fi

    UN=`samtools view -c -f 4 $f`
    if [ "$UN-" == "0-" ]; then
	echo "ERROR: No unaligned reads found in $f." > /dev/stderr
	exit 1
    fi

    named_pipe=.`basename .$f`.pipe.fastq
    mkfifo $named_pipe
    echo "Converting BAM to fastq"
    samtools bam2fq $f > $named_pipe &
    
    FILES2PROCESS=$named_pipe
    FILES2DELETE=$named_pipe
else
    FILES2PROCESS=
    FILES2DELETE=
    for f in $FILES; do
	ext=`file_extension $f`
	if [ "-$ext" == "-" ]; then
	    ext=$(file_type $f)
	    echo "File $f does not have an extension, assuming that it is '.$ext'"
	fi
	if [ "-$ext" == "-bz2" ] || [ "-$ext" == "-bzip2" ] ; then
	    echo BZIP file
	    # check integrity
	    set +e
	    echo "Checking integrity of $f..."
	    bzip2  -t $f 
	    if [ $? -ne 0 ]; then
		echo "ERROR: $f: error uncompressing bzip2 file"
		exit 2
	    fi
	    set -e
	    echo "Checking integrity of $f...complete."
	    named_pipe=.`basename .$f`.pipe.fastq
	    rm -f $named_pipe
	    mkfifo $named_pipe
	    bunzip2 -k  -c $f > $named_pipe  &
	    FILES2PROCESS="$FILES2PROCESS $named_pipe"
	    FILES2DELETE="$FILES2DELETE $named_pipe"
	else
	    FILES2PROCESS="$FILES2PROCESS $f"
	fi
    done
fi
fastq_info $FILES2PROCESS $PE_PARAMETER

if [ "-$FILES2DELETE" != "-" ]; then
    #echo -n "Removing named pipes..."
    rm -f $FILES2DELETE
    #echo "done."
fi
exit 0
