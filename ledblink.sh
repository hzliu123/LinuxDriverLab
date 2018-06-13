#!/bin/bash
COUNTER=0

echo 18 > /sys/class/gpio/export
sleep 1

echo high > /sys/class/gpio/gpio18/direction
sleep 1

while [ $COUNTER -lt 10 ]; do
  echo 0 > /sys/class/gpio/gpio18/value
  sleep 1
  echo 1 > /sys/class/gpio/gpio18/value
  sleep 1
  echo The counter is $COUNTER
  let COUNTER=COUNTER+1
done

echo 18 > /sys/class/gpio/unexport
