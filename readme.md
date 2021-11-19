# Networks and Systems Programation COM SUCKET
## Features we may want :
- Nagle or Clark algorithm to help with flow control
- Congestion control
    - Slow Start
    - CSMA/CD
    - Fast Retransmit
    - Fast Recovery
    - CUBIC ??
- The things we need to mesure/estimate/set :
    - MSS (maximum segment size), la taille maximum d'un segment (données TCP uniquement);
    - awnd (advertised window), fenêtre de réception;
    - cwnd (congestion window), fenêtre de congestion;
    - swnd (sending window), fenêtre d'émission (= min (cwnd, awnd));
    - RTO (retransmit timeout), temps de transmission maximal (souvent 2 fois le RTT);
    - RTT (roundtrip time), durée d'envoi puis de réception d'acquittement d'un paquet;
    - ssthresh (slow start threshold), taille maximum d'une fenêtre d'émission en slow start;
    - LFN (long fat network), réseau avec une grande bande passante mais aussi un long délai de réponse;

![cum socket](https://i.redd.it/ft3rlt7svjy21.jpg)

## Rules :
1. Le client utilise des sockets UDP pour transférer un fichier depuis le serveur.
2. Le client commence par la transmission d’un message contenant la chaine de ca-
ractères "SYN" sur le port public du serveur.
3. Le client attend le message "SYN-ACKPPPP" de la part du serveur, où "PPPP"
représente le numéro d’un nouveau port, utilisé pour l’échange de données (le
numéro de port doit être entre 1000 et 9999).
4. Le client répond avec un message "ACK" sur le port public.
5. Juste après la fin de la connexion, le client envoie un message contenant le nom
du fichier recherché au serveur, sur le port communiqué par celui-ci dans la phase
de connexion.
6. Le client attend des messages de données qui commencent avec un numéro de
séquence, en format chaine de caractères, sur 6 octets.
7. Quand le message est bien reçu, le client répond avec un message "ACKSSSSSS",
où "SSSSSS" est le numéro de séquence du dernier segment reçu en contigu. Ce
message est transmis sur le port dédié au client.
8. A la fin de la transmission du fichier, le client attend un message "FIN" de la part
du serveur.