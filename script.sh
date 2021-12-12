#!/bin/bash
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ] 
then
    echo "the syntax is : ./script.sh server_ip server_port file_name" 
else
    ((i = 0))

    for (( ; ; )) 
    do
        echo [launching client] ...... attempt number $i
        ./client1 $1 $2 $3        
        i=$((i+1))
    done
fi