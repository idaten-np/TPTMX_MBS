#!/bin/bash
isfp=1
		ireg=0x00
			command="w"					## 'r'ead or 'w'rite
			thres=0x4908				## threshold value. default: 0x5208
			#thres=0x4E08				## threshold value. default: 0x5208
			#thres=0x5208
for idev in {0..6}; do					## tamex board
	for ich in {0..1}; do				## (channel)/8
		for isubreg in {0..7}; do		## (channel)%8
			echo "./pqdc1_spi.py $isfp $idev $ich $ireg $isubreg $command $thres"
			./pqdc1_spi.py $isfp $idev $ich $ireg $isubreg $command $thres
		done;
	done;

done;


