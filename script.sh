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
        read -ra array <<< "$(ps -o pid,args -C client1 | grep client1)"
        echo $array[1]
        ((ar=$(echo "$array[1]" | sed 's/.$//' | sed 's/.$//' | sed 's/.$//'))
        echo "this is the client PID: $ar"
        sleep 5
        kill -9 $ar

        i=$((i+1))
 
    done
fi