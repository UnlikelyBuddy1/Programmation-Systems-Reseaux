#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#define MTU 1500
#define LOG 0

unsigned long RTT_ESTIMATION(float alpha, unsigned long SRTT, unsigned long RTT)
{ 
    return SRTT = alpha*(SRTT)+(1-alpha)*RTT;
}
char pass(){
    return ' ';
}
void verifyArguments(int argc, char *argv[], unsigned short *port_udp_con){
    if(argc > 2){
        perror("[ERR] Too many arguments\n");
        exit(0);
    }
    if(argc < 2){
        perror("[ERR] Argument expected\n");
        exit(0);
    }
    if(argc == 2){
        *port_udp_con = atoi(argv[1]);
    }
    (LOG)?(printf("[OK] Port used is %d\n", *port_udp_con)):(pass());
}

int createSocket(){
    int udpSocket;
    udpSocket=socket(AF_INET, SOCK_DGRAM, 0);
    if(udpSocket < 0){
        perror("[ERR] Cannot create udp socket\n");
        exit(0);
    } else{
        (LOG)?(printf("[OK] Socket server UDP created %d\n", udpSocket) ):(pass());
    }
    return udpSocket;
}

int bindSocket(int udpSocket, unsigned short port_udp){
    struct sockaddr_in adresse;
    memset(&adresse, 0, sizeof(adresse)); 
    adresse.sin_family= AF_INET;
    adresse.sin_port= htons(port_udp);    
    adresse.sin_addr.s_addr= INADDR_ANY;
    (LOG)?(printf("[OK] Created adresse with port %d and addr %d\n", adresse.sin_port, adresse.sin_addr.s_addr)):(pass());
    if (bind(udpSocket, (struct sockaddr*) &adresse, sizeof(adresse))<0) {
        perror("[ERR] Bind failed\n");
        close(udpSocket);
        return -1;
    }
    (LOG)?(printf("[OK] Binded server %d to adress with family %d, port %d, addr %d\n",udpSocket, adresse.sin_family, adresse.sin_port,adresse.sin_addr.s_addr)):(pass());
    return udpSocket;
}

void TWH(int udpSocket, int n, char buffer[], unsigned short port_udp, struct sockaddr_in cliaddr, socklen_t len){
    struct timespec begin, end; 

    (LOG)?(printf("[INFO] Begining three way handshake waiting to receive SYN(blocking)\n")):(pass());
    n = recvfrom(udpSocket, (char *)buffer, MTU,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0';
    (LOG)?(printf("[OK] buffer_con to receive created %d\n", n)):(pass());
    if(strcmp(buffer,"SYN")==0){
        (LOG)?(printf("[INFO] The buffer_con is :\n")):(pass());
        (LOG)?(printf("[DATA] %s\n", buffer)):(pass());
        char synack[11];
        char port_udp_data_s[6];
        clock_gettime(CLOCK_REALTIME, &begin);
        strcpy(synack, "SYN-ACK");
        snprintf((char *) port_udp_data_s, 10 , "%d", port_udp ); 
        strcat(synack,port_udp_data_s);
        strcpy(buffer, synack);
        (LOG)?(printf("[INFO] Sending for SYN-ACK...\n")):(pass());
        (LOG)?(printf("[INFO] The buffer_con is :\n")):(pass());
        (LOG)?(printf("[DATA] %s\n", buffer)):(pass());
        // En multi client il faut creer le thread avant de envoyer le synack
        // faire lecture fichier -> envoi -> réecriture
        sendto(udpSocket, (char *)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        (LOG)?(printf("[INFO] Waiting for ACK ...\n")):(pass());
        n = recvfrom(udpSocket, (char *)buffer, MTU,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        (LOG)?(printf("[INFO] The buffer_con is :\n")):(pass());
        (LOG)?(printf("[DATA] %s\n", buffer)):(pass());
        if(strcmp(buffer,"ACK")==0){
            (LOG)?(printf("[OK] ACK received, succesful connection\n")):(pass());
            clock_gettime(CLOCK_REALTIME, &end);
        // Handshake reussi !

        }else{
        printf("[ERR] Bad ack");
        exit(0);
        }
    }else{
        printf("[ERR] Bad SYN");
        exit(0);
    }
    unsigned long microseconds = ((end.tv_sec*1e6) + (end.tv_nsec/1e3)) - ((begin.tv_sec*1e6) + (begin.tv_nsec/1e3));
    printf("[INFO] Time measured: %ld µs.\n", microseconds);
    return ;
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

unsigned long getNumberFragments(size_t length, unsigned long *pLastFrag){
    unsigned long numberFrags;
    if((length % (MTU-6)) != 0){
        numberFrags = (length/(MTU-6))+1;
        *pLastFrag = (length % (MTU-6));
    } else {
        numberFrags = length/(MTU-6);
    }        
    (LOG)?(printf("[OK] There needs to be %lu fragments\n",numberFrags)):(pass());
    (LOG)?(printf("[INFO] size at the end will be %d\n", *pLastFrag)):(pass());
    return numberFrags;
}

FILE *verifyFile(char buffer[], int size){
    FILE *file;
    char filename[size];
        strcpy(filename, buffer);
        (LOG)?(printf("[OK] Message is : %s\n", buffer)):(pass());
        file = NULL;
        if((file=fopen(filename,"r+"))==NULL){
            printf("[ERR] File %s does not exist\n", filename);
            exit(-1);
        } else {
            (LOG)?(printf("[OK] Found file %s\n", filename)):(pass());
        }
    return file;
}