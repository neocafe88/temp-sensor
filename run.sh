#!/usr/bin/env bash

set -e

printf "\nRunning temp sensor...\n"

./tempsensor --endpoint arehxr1om855h-ats.iot.us-east-1.amazonaws.com --cert BBBTempSensor.cert.pem --key BBBTempSensor.private.key --ca_file root-CA.crt --client_id tempsensor --topic sdk/test/temp


