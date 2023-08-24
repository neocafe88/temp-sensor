#!/bin/sh

set -e

printf "\nRunning temp sensor...\n"

tempsensor --endpoint arehxr1om855h-ats.iot.us-east-1.amazonaws.com --cert /etc/temp-sensor/BBBTempSensor.cert.pem --key /etc/temp-sensor/BBBTempSensor.private.key --ca_file /etc/temp-sensor/root-CA.crt --client_id tempsensor --topic sdk/test/temp


