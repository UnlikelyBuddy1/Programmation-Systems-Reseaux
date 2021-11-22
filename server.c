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
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, nFrags, errors=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data;// les deux port UDP utilise
    char buffer_con[RCVSIZE], buffer_data[RCVSIZE], StringSeqNumber[7]; // le buffer_con ou on va recevoir des donnees et des connexion
    int udp_con, udp_data, n=0, ack = 0;
    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len);
    struct timeval tv = setTimer(0,10000);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));
    
    while(1) {
        printf("[INFO] Waiting for message being name of file to send ...\n");
        n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
        file = verifyFile(buffer_data, sizeof(buffer_data));
        size_t length = getLengthFile(file);
        char buffer2_lecture[length];
        if((fread(buffer2_lecture,sizeof(char),length,file))!=length){printf("[ERR] Failed to read file\n");} 
        fclose(file);
        nFrags = getNumberFragments(length, pLastFrag);
       
        for(int seqNumber=0;seqNumber <= nFrags;seqNumber++){
            memcpy(buffer_data+6, buffer2_lecture+((RCVSIZE-6)*seqNumber), RCVSIZE-6);
            snprintf((char *) StringSeqNumber, 7 , "%6d", seqNumber );
            memcpy(buffer_data, StringSeqNumber, 6);
            printf("[INFO] Sending file %d/%d ...",seqNumber, nFrags-1);
            //verifyContents(buffer_data, buffer2_lecture, 0, 20);
            ack = 0;
            while(ack == 0){
                sendto(udp_data,(const char*)buffer_data, (seqNumber==nFrags)?(lastFragSize):(sizeof(buffer_data)),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
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
        printf("there have been %d errors in %d messages\n", errors, nFrags);
    }        
    return 0;
}