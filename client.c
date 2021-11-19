#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define RCVSIZE 508


int main (int argc, char *argv[]) {
    
    struct sockaddr_in adresse;
    //FILE *file;
    int port,port2 = 5001;
    char msg[RCVSIZE];
    char *buffer_ecriture = malloc (sizeof(char)*104857600);

    if(argc > 3){
        printf("[ERR] Too many arguments\n");
        exit(0);
    }
    if(argc < 3){
        printf("[ERR] Argument expected\n");
        exit(0);
    }
    if(argc == 3){
        port = atoi(argv[2]);
    }
    
    int server_desc = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&adresse, 0, sizeof(adresse));
    if (server_desc < 0) {
        printf("[ERR] cannot create socket\n");
        return -1;
    }
        
    adresse.sin_family= AF_INET;
    adresse.sin_port= htons(port);
    adresse.sin_addr.s_addr= inet_addr(argv[1]);

    socklen_t len = sizeof(adresse);
    int  n; 

    printf("[INFO] Begining SYNC process ...\n");
    strcpy(msg, "SYN");
    sendto(server_desc,(const char*)msg, strlen(msg),MSG_CONFIRM, (const struct sockaddr *) &adresse,sizeof(adresse));
    printf("[OK] Sent SYNC\n");

    n = recvfrom(server_desc, (char *)msg, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &adresse,&len);
    printf("%d\n",ntohs(adresse.sin_port));
    msg[n] = '\0';
        
    char *token;
    token = strtok(msg, "SYN-ACK");
    printf("[OK] Token is %s\n", token);
    if(token != NULL){
        port2 =  atoi(token);
        printf("[OK] SYN-ACK received\n");
        printf("[INFO] Port read in SYN ACK is : %d\n",port2);
        strcpy(msg, "ACK");
        sendto(server_desc,(const char*)msg, strlen(msg),MSG_CONFIRM, (const struct sockaddr *) &adresse,sizeof(adresse));
        printf("[INFO] Sending ack, Client connecting\n");
    }else{
        printf("[ERR] SYN-ACK failed\n");
        exit(0);
    }
    adresse.sin_port = htons(port2);
    int fini = 0;
    char nom_fichier[20];
    char num_seq_s[7];
    int num_seq;
    char ack_s[11];

    while(1){
        fgets(msg, RCVSIZE, stdin);
        strcpy(msg,strtok(msg, "\n"));
        strcpy(nom_fichier,msg);
        printf("[OK] Name of file is : %s\n",nom_fichier); 
        sendto(server_desc,(const char*)msg, strlen(msg),MSG_CONFIRM, (const struct sockaddr *) &adresse,sizeof(adresse)); 
        n = recvfrom(server_desc, (char *)msg, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &adresse,&len);
        if (strstr(msg, "ACK") != NULL) {
            printf("[OK] ACK received\n");
        } else if (strstr(msg, "NAC") != NULL) {
            printf("[ERR] sever can't find file or connection is lost");
        }
        fini = 0;
        
        while(fini == 0){
            
            n = recvfrom(server_desc, (char *)msg, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &adresse,&len);
            msg[n]='\0';
           
            if(strcmp(msg,"FIN")==0){
                printf("[OK] Received EOF signal\n");
                fini = 1;
                sendto(server_desc,"ACK", strlen("ACK"),MSG_CONFIRM, (const struct sockaddr *) &adresse,sizeof(adresse));   
            }else{
                strcpy(ack_s,"ACK_");
                memcpy(num_seq_s,msg,6);
                num_seq = atoi(num_seq_s);
                memcpy(buffer_ecriture+(RCVSIZE-6)*num_seq,msg,RCVSIZE-6);

                strcat(ack_s,num_seq_s);
                strcpy(msg, ack_s);   
                //printf("Acquitement : %s , no : %d\n",msg,num_seq);             
                sendto(server_desc,(const char*)msg, strlen(msg),MSG_CONFIRM, (const struct sockaddr *) &adresse,sizeof(adresse));
            }
        }   
    }
    free(buffer_ecriture);
    return 0;
}