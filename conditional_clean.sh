#!/bin/bash

if [ ! -f "Makefile" ]; then
    echo "Invalid diretory, no Makefile."
    exit
fi

FILE=last_build_dtls_enabled

if [ ! -f "$FILE" ]; then
    echo "DTLS state unknown, cleaning..."
    make clean
    echo "$GCOAP_ENABLE_DTLS" >$FILE
    exit
fi

if [ "$GCOAP_ENABLE_DTLS" == "$(cat $FILE)" ]; then
    echo "DTLS same as the last time"
else
    echo "DTLS is NOT as the last time! Cleaning..."
    make clean
    echo "$GCOAP_ENABLE_DTLS" >$FILE
fi
