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
#include "tools.h"

int main (int argc, char *argv[]) {
    FILE* file;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, nFrags, errors=0, toSend=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data;// les deux port UDP utilise
    char buffer_con[RCVSIZE], buffer_data[RCVSIZE]; // le buffer_con ou on va recevoir des donnees et des connexion
    int udp_con, udp_data, n=0, ack = 0;
    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len);
    struct timeval tv = setTimer(0,10000);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));
    
    FILE *log;
    log = fopen("log.txt", "w");
    while(1){
        printf("[INFO] Waiting for message being name of file to send ...\n");
        n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
        file = verifyFile(buffer_data, sizeof(buffer_data));
        size_t length = getLengthFile(file);
        char buffer_file[RCVSIZE-6];
        nFrags = getNumberFragments(length, pLastFrag);
       
        for(int seqNumber=0;seqNumber < nFrags;seqNumber++){
            toSend=fread(buffer_file, 1, (seqNumber==nFrags-1)?(lastFragSize):(RCVSIZE-6), file);
            memset(buffer_data, 0, RCVSIZE);
            sprintf(buffer_data, "%6d", seqNumber);
            memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
            fprintf(log, "%s", buffer_data);
            printf("[INFO] Sending file %d/%d ...\r", seqNumber, nFrags);
            //verifyContents(buffer_data, buffer_file, 0, 20);
            ack = 0;
            while(ack == 0){
                sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);   
                if(strstr(buffer_data, "ACK") != NULL) {
                    if(atoi(strtok(buffer_data,"ACK")) == seqNumber){
                        ack = 1;
                        printf("[OK] Received ACK %d/%d\r",seqNumber, nFrags);
                    }
                }
                else{
                    errors+=1;
                }
            }
            
        }
        printf("\n");
        sendto(udp_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
        printf("[INFO] Waiting for message FINAL ACK to end connection ...\n");
        n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len); 
        printf("[OK] Received FINAL ACK\n");
        fclose(file);
        fclose(log);
        printf("there have been %d errors in %d messages\n", errors, nFrags);
    }        
    return 0;
}