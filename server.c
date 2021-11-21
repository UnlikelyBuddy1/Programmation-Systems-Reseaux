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

#define RCVSIZE 508

int main (int argc, char *argv[]) {
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data;// les deux port UDP utilise
    char buffer_con[RCVSIZE]; // le buffer_con ou on va recevoir des donnees de connexion
    char buffer_data[RCVSIZE]; // le buffer_con ou on va recevoir des donnees
    int server_desc_udp, server_desc_udp2, n=0;
    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    server_desc_udp = createSocket();
    server_desc_udp = bindSocket(server_desc_udp, port_udp_con);
    server_desc_udp2 = createSocket();
    server_desc_udp2 = bindSocket(server_desc_udp2, port_udp_data);
    TWH(server_desc_udp, n, buffer_con, port_udp_data, cliaddr, len);

    FILE* fichier;
    struct timeval tv = setTimer(0,10000);
    setsockopt(server_desc_udp2, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));
    unsigned short errors=0;
    
    while(1) {
        fichier = receiveFilename(server_desc_udp2, buffer_data, cliaddr, len, n);
        size_t length = getLengthFile(fichier);
        char buffer2_lecture[length];
        
        if((fread(buffer2_lecture,sizeof(char),length,fichier))!=length){
            printf("[ERR] Failed to read file\n");
        }
        fclose(fichier);
        
        unsigned short lastFragSize, *pLastFrag=&lastFragSize;
        unsigned int nFrags = getNumberFragments(length, pLastFrag);

        int ack = 0;
        char num_seq_tot[7];
        char num_seq_s[7];
        
        printf("[INFO] size at the end will be %d\n", lastFragSize);
        printf("[INFO] buffer_data size is : %d\n", n);
        buffer_data[n] = '\0';

        for(int num_seq=0;num_seq < nFrags;num_seq++){
            memcpy(buffer_data+6, buffer2_lecture+((RCVSIZE-6)*num_seq), RCVSIZE-6);
            strcpy(num_seq_tot, "000000");
            snprintf((char *) num_seq_s, 10 , "%d", num_seq );
            for(int i = strlen(num_seq_s);i>=0;i--){
                num_seq_tot[strlen(num_seq_tot)-i]=num_seq_s[strlen(num_seq_s)-i];
            }
            memcpy(buffer_data,num_seq_tot, 6);
            printf("[INFO] Sending file %d/%d ...",num_seq, nFrags-1);
            //printf("[INFO]Copying %d bytes from %s to %s\r", RCVSIZE-6, buffer2_lecture+((RCVSIZE-6)*num_seq), buffer_data+6);
            printf("[OK] Sequence number is %s ...", num_seq_tot);
            ack = 0;
            while(ack == 0){
                sendto(server_desc_udp2,(const char*)buffer_data, (num_seq==nFrags)?(lastFragSize):(sizeof(buffer_data)),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                n = recvfrom(server_desc_udp2, (char *)buffer_data, RCVSIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr,&len); 
                buffer_data[n] = '\0';   
                printf("this is what we received %s, %d\n", buffer_data, num_seq);
                if(strstr(buffer_data, "ACK") != NULL) {
                    if(atoi(strtok(buffer_data,"ACK")) == num_seq){
                        ack = 1;
                        printf("[OK] Received ACK %d/%d\r",num_seq, nFrags);
                    }
                }
                else{
                    errors+=1;
                }
            }
        }
        // ToDo retransmission si pas ack

        sendto(server_desc_udp2,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
        printf("[INFO] Waiting for message FINAL ACK to end connection ...\n");
        n = recvfrom(server_desc_udp2, (char *)buffer_data, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len); 
        buffer_data[n] = '\0';
        printf("[OK] Received FINAL ACK\n");
        printf("there have been %d errors in %d messages\n", errors, nFrags);
    }        
    return 0;
}
