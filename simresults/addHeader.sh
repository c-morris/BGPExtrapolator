#!/bin/sh

filename=${1};
newFile="${filename}_new";

cat ./header.csv $filename > ${newFile};
rm $filename;
mv $newFile $filename;
