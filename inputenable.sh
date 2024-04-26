#!/bin/bash
			
#./pqdc1_spi.py	$isfp	$itam	$ich/8	0x00	$ich%8	r/w	 val_threshold
# 0xffff enable all 0x0000 disable all
 ./pqdc1_spi.py	1	0	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	0	1	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	1	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	1	1	0x20	0	w	 0Xffff
 ./pqdc1_spi.py	1	2	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	2	1	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	3	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	3	1	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	4	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	4	1	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	5	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	5	1	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	6	0	0x20	0	w	 0xffff
 ./pqdc1_spi.py	1	6	1	0x20	0	w	 0xffff
