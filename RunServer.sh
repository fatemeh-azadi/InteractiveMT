#!/bin/bash

dir=`pwd`
MosesAdd=$dir/FinalCAT/bin
MosesConfig=$dir/Models/TM/moses-tuned-bin.ini
OutputDir=$dir/Outputs

mkdir -p $OutputDir

Port=$1   ## Port number used for running server on
Method=WeightedCaitraWithJump
JumpLen=5
ErrorWeight=-0.5
BackOff=0
###################################################################################################################
#####				 Server Parameters are passed as input			          		###
###														###
###  -p : the port number for running server on it (default = 9999)		    		  		###
###														###
#### Searching Algorithm Parameters:										###
###  -method or -m: the method used for searching the best suffix (default = caitra) 			   	###
##       	 Caitra|ModifiedCaitra|WeightedCaitra|CaitraWithJump|WeightedCaitraWithJump    		    	###
###  --ErrorWeight or -errw: weight of the error model while adding to translation scores (default = -0.5) 	###
###  --jumpLen: the maximum number of nodes allowed for jump (default = 10000)					###
###														###
###														###
#### BackOff Model Parameters:											###
###  --backoff: the model used for backoff (default = 0)							###
##              0(no-backoff)|1(LM-Backoff)|2(wordvector-Backoff)|3(LM-then-WordVector-Backoff)			###
###  --backOffNgram: n-gram order used for the LM backoff (default = 4)						###
###  --WordVector: address of the trained wordvector model							###
###														###
###														###
#### Online Learning Parameters:										###
###  --onlineMethod or -o: the method used for learning feature weights online	(default = none)		###
##			none|Perceptron|Mira|PA2								###
###  --onlineMetric: the metric used for tuning feature weights (default = BLEU)				###
##			BLEU|TER|WER|KSMR									###
###  -wlr: the learning rate used in online algorithms (default = 0.2)						###
###  --nbest-size: the n-best size used for tuning feature weights (default = 100)				###
###################################################################################################################

InputParameters="-p $Port -m $Method --jumpLen $JumpLen --ErrorWeight $ErrorWeight --backOff $BackOff"

$MosesAdd/cat-server -config $MosesConfig  -osg $OutputDir/osg-simple <<< $InputParameters 2> $OutputDir/Server-Log > $OutputDir/Server-Output
