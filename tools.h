#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#define RCVSIZE 508

void verifyArguments(int argc, char *argv[], unsigned short *port_udp_con, unsigned short *port_udp_data){
    if(argc > 2){
        perror("[ERR] Too many arguments\n");
        exit(0);
    }
    if(argc < 2){
        perror("[ERR] Argument expected\n");
        exit(0);
    }
    if(argc == 2){
        int n = 1;
        *port_udp_con = atoi(argv[1]);
        *port_udp_data = atoi(argv[1])+n;
        n++;
    }
    printf("[OK] Ports used are %d and %d\n", *port_udp_con, *port_udp_data);
}

int createSocket(){
    int udpSocket;
    udpSocket=socket(AF_INET, SOCK_DGRAM, 0);
    if(udpSocket < 0){
        perror("[ERR] Cannot create udp socket\n");
        exit(0);
    } else{
       printf("[OK] Socket server UDP created %d\n", udpSocket); 
    }
    return udpSocket;
}

int bindSocket(int udpSocket, unsigned short port_udp){
    struct sockaddr_in adresse;
    memset(&adresse, 0, sizeof(adresse)); 
    adresse.sin_family= AF_INET;
    adresse.sin_port= htons(port_udp);    
    adresse.sin_addr.s_addr= INADDR_ANY;
    printf("[OK] Created adresse with port %d and addr %d\n", adresse.sin_port, adresse.sin_addr.s_addr);
    if (bind(udpSocket, (struct sockaddr*) &adresse, sizeof(adresse))<0) {
        perror("[ERR] Bind failed\n");
        close(udpSocket);
        return -1;
    }
    printf("[OK] Binded server %d to adress with family %d, port %d, addr %d\n",udpSocket, adresse.sin_family, adresse.sin_port,adresse.sin_addr.s_addr);
    return udpSocket;
}

void TWH(int udpSocket, int n, char buffer[], unsigned short port_udp, struct sockaddr_in cliaddr, socklen_t len){
    printf("[INFO] Begining three way handshake waiting to receive SYN(blocking)\n");
    n = recvfrom(udpSocket, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0';
    printf("[OK] buffer_con to receive created %d\n", n);
    
    if(strcmp(buffer,"SYN")==0){
        printf("[INFO] The buffer_con is :\n"); 
        printf("[DATA] %s\n", buffer);
        char synack[11];
        char port_udp_data_s[6];
        strcpy(synack, "SYN-ACK");
        snprintf((char *) port_udp_data_s, 10 , "%d", port_udp ); 
        strcat(synack,port_udp_data_s);
        strcpy(buffer, synack);
        printf("[INFO] Sending for SYN-ACK...\n");
        printf("[INFO] The buffer_con is :\n");
        printf("[DATA] %s\n", buffer);
        // En multi client il faut creer le thread avant de envoyer le synack
        // faire lecture fichier -> envoi -> réecriture
        sendto(udpSocket, (char *)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        printf("[INFO] Waiting for ACK ...\n");
        n = recvfrom(udpSocket, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        printf("[INFO] The buffer_con is :\n");
        printf("[DATA] %s\n", buffer);
        if(strcmp(buffer,"ACK")==0){
            printf("[OK] ACK received, succesful connection\n");
        // Handshake reussi !

        }else{
        printf("[ERR] Bad ack");
        exit(0);
        }
    }else{
        printf("[ERR] Bad SYN");
        exit(0);
    }
}

size_t getLengthFile(FILE *file){
    size_t pos = ftell(file);    // Current position
    fseek(file, 0, SEEK_END);    // Go to end
    size_t length = ftell(file); // read the position which is the size
    fseek(file, pos, SEEK_SET);
    printf("[INFO] File has a size of %lu bytes\n",length);
    return length;
}

struct timeval setTimer(unsigned int seconds, unsigned int micro_seconds){
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = micro_seconds;
    return tv;
}

unsigned int getNumberFragments(size_t length, unsigned short *pLastFrag){
    unsigned int numberFrags;
    if((length % (RCVSIZE-6)) != 0){
        numberFrags = (length/(RCVSIZE-6))+1;
        *pLastFrag = (length % (RCVSIZE-6));
    } else {
        numberFrags = length/(RCVSIZE-6);
    }        
    printf("[OK] There needs to be %d fragments\n",numberFrags);
    return numberFrags;
}

FILE *verifyFile(char buffer[], int size){
    FILE *file;
    char filename[size];
        strcpy(filename, buffer);
        printf("[OK] Message is : %s\n", buffer);
        file = NULL;
        if((file=fopen(filename,"r"))==NULL){
            printf("[ERR] File %s does not exist\n", filename);
            exit(-1);
        } else {
            printf("[OK] Found file %s\n", filename); 
        }
    return file;
}