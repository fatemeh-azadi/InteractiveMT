#!/bin/bash

Port=$1   ## the port number that the server is running on
dir=`pwd`
MosesAdd=$dir/FinalCAT/bin
TestFile=$dir/Corpora/Test/test.fa0
RefFile=$dir/Corpora/Test/test.en
OutputDir=$dir/Outputs

mkdir $OutputDir

$MosesAdd/cat-client -i $TestFile -r $RefFile -o $OutputDir/Client-Log -p $Port

