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
void printArray(char array[]);

int main (int argc, char *argv[]) {

    struct sockaddr_in adresse, cliaddr, adresse2; // ici description des adresse
    memset(&cliaddr, 0, sizeof(cliaddr)); // en struct et allocation memoire
    memset(&adresse, 0, sizeof(adresse)); 
    memset(&adresse2, 0, sizeof(adresse)); 
    
    
    int port_udp, port_udp2= 5002; // les deux port UDP utilise
    char buffer[RCVSIZE]; // le buffer ou on va recevoir des donnees


    if(argc > 2){
        printf("[ERR] Too many arguments\n");
        exit(0);
    }
    if(argc < 2){
        printf("[ERR] Argument expected\n");
        exit(0);
    }
    if(argc == 2){
        int n = 1;
        port_udp = atoi(argv[1]);
        port_udp2 = atoi(argv[1])+n;
        n++;
    }

    printf("[OK] Ports used are %d and %d\n", port_udp, port_udp2);
    

    int server_desc_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_desc_udp < 0){
        perror("[ERR] Cannot create udp socket\n");
        return -1;
    }
    printf("[OK] Socket server UDP created %d\n", server_desc_udp);
    
    adresse.sin_family= AF_INET;
    adresse.sin_port= htons(port_udp);    
    adresse.sin_addr.s_addr= INADDR_ANY;
    printf("[OK] Created adresse with port %d and addr %d\n", adresse.sin_port, adresse.sin_addr.s_addr);

    adresse2.sin_family= AF_INET;
    adresse2.sin_port= htons(port_udp2);    
    adresse2.sin_addr.s_addr= INADDR_ANY;
    printf("[OK] Created adresse with port %d and addr %d\n", adresse2.sin_port, adresse2.sin_addr.s_addr);

    if (bind(server_desc_udp, (struct sockaddr*) &adresse, sizeof(adresse))<0) {
        perror("[ERR] Bind failed\n");
        close(server_desc_udp);
        return -1;
    }
    printf("[OK] Binded server %d to adress with family %d, port %d, addr %d\n",server_desc_udp, adresse.sin_family, adresse.sin_port,adresse.sin_addr.s_addr);

    socklen_t len = sizeof(cliaddr);
    printf("[INFO] Length of client address is %d\n",len);
    int n;
    printf("[INFO] Begining three way handshake waiting to receive something (blocking)\n");
    n = recvfrom(server_desc_udp, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0';
    printf("[OK] Buffer to receive created %d\n", n);
    
    if(strcmp(buffer,"SYN")==0){
        printf("[INFO] The buffer is :\n");
        printArray(buffer); 
        char synack[13];
        char port_udp2_s[6];
        strcpy(synack, "SYN-ACK");
        snprintf((char *) port_udp2_s, 10 , "%d", port_udp2 ); 
        strcat(synack,port_udp2_s);
        strcpy(buffer, synack);
        printf("[INFO] Sending for SYN-ACK...\n");
        printf("[INFO] The buffer is :\n");
        printArray(buffer);
        // En multi client il faut creer le thread avant de envoyer le synack
        // faire lecture fichier -> envoi -> réecriture
        sendto(server_desc_udp, (char *)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        printf("[INFO] Waiting for ACK ...\n");
        n = recvfrom(server_desc_udp, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        printf("[INFO] The buffer is :\n");
        printArray(buffer); 
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

    int server_desc_udp2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(server_desc_udp2, (struct sockaddr*) &adresse2, sizeof(adresse2))<0) {
        perror("[ERR] Bind failed\n");
        close(server_desc_udp);
        exit(0);
    }
    printf("[OK] Binded server %d to adress with family %d, port %d, addr %d\n",server_desc_udp2, adresse2.sin_family, adresse2.sin_port,adresse2.sin_addr.s_addr);
    
    FILE* fichier;
    
    while(1) {
        printf("[INFO] Waiting for message being name of file to send ...\n");
        
        n = recvfrom(server_desc_udp2, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len); 
        buffer[n] = '\0';
        char filename[sizeof(buffer)];
        strcpy(filename, buffer);
        printf("[OK] Message is : %s\n", filename);

        fichier = NULL;
        char fileACK[3]={'A', 'C', 'K'};
        if((fichier=fopen(filename,"r"))==NULL){
            printf("[ERR] File %s does not exist\n", filename);
            strcpy(fileACK, "NAC");
        } else {
            printf("[OK] Found file %s\n", filename); 
        }
          
        char sendBuffer[n];
        strcpy(sendBuffer, fileACK);
        printf("[INFO] Sending %s..\n", fileACK);
        sendto(server_desc_udp2, (char *)sendBuffer, strlen(sendBuffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        printf("[OK] Sent %s\n", fileACK);   
        
        if(fileACK=="NAC"){
            exit(-1);
        }
        stpcpy(buffer, filename);

        size_t pos = ftell(fichier);    // Current position
        fseek(fichier, 0, SEEK_END);    // Go to end
        size_t length = ftell(fichier); // read the position which is the size
        fseek(fichier, pos, SEEK_SET);
        char buffer_lecture[length];

        if((fread(buffer_lecture,sizeof(char),length,fichier))!=length){
            printf("[ERR] Failed to read file\n");
        }
        fclose(fichier);
        
        printf("[INFO] File has a size of %d bytes\n",length);

        int nb_morceaux;
        printf("[INFO] Checking if fragmentation is needed ...\n",length);
        if((length % (RCVSIZE-6)) != 0){
            nb_morceaux = (length/(RCVSIZE-6))+1;
        } else {
            nb_morceaux = length/(RCVSIZE-6);
        }        
        printf("[OK] There needs to be %d fragments\n",nb_morceaux);

        int ack = 0;
        char num_seq_tot[7];
        char num_seq_s[7];

        printf("[INFO] Buffer size is : %d\n", n);
        buffer[n] = '\0';
        printf("\n");
        for(int num_seq=0;num_seq < nb_morceaux;num_seq++){
            memcpy(buffer+6, buffer_lecture+((RCVSIZE-6)*num_seq), RCVSIZE-6);
            strcpy(num_seq_tot, "000000");
            snprintf((char *) num_seq_s, 10 , "%d", num_seq );
            for(int i = strlen(num_seq_s);i>=0;i--){
                num_seq_tot[strlen(num_seq_tot)-i]=num_seq_s[strlen(num_seq_s)-i];
            }
            memcpy(buffer,num_seq_tot, 6);
            printf("[INFO] Sending file %d/%d ...\r",num_seq, nb_morceaux-1);
            printf("[INFO]Copying %d bytes from %x to %x\r", RCVSIZE-6, buffer_lecture+((RCVSIZE-6)*num_seq), buffer+6);
            printf("[OK] Sequence number is %s\r", num_seq_tot);
            ack = 0;
            while(ack == 0){
                sendto(server_desc_udp2,(const char*)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                n = recvfrom(server_desc_udp2, (char *)buffer, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len); 
                buffer[n] = '\0';          
                if(atoi(strtok(buffer,"ACK_")) == num_seq){
                    ack = 1;
                    printf("[OK] Received ACK %d/%d\r",num_seq, nb_morceaux);
                }
            }
        }
        printf("\n");
        // ToDo retransmission si pas ack

        sendto(server_desc_udp2,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
        printf("[INFO] Waiting for message FINAL ACK to end connection ...\n");
        n = recvfrom(server_desc_udp2, (char *)buffer, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len); 
        buffer[n] = '\0';
        printf("[OK] Received FINAL ACK\n");
    }        
    return 0;
}

void printArray(char array[]){
    int length = sizeof(array);
    printf("[MSG] ");
    for(int i=0; i<length; i++){
        printf("%c", array[i]);
    }
    printf("\n");
}