#! /bin/bash
#ps_out= $(ps -o pid,args -C client1 | grep client1)
        
        read -ra array <<< "$(ps -o pid,args -C client1 | grep client1)"
        echo "this is array: $array[1]"
        ((ar= $(echo "$array[1]" | sed 's/.$//' | sed 's/.$//' | sed 's/.$//')))
        echo "this ar $ar"
        