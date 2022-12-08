#!/bin/bash

while true
do
	aplay -Dhw:1,0 /root/m1-server/piano.wav
	sleep 0.5
done


