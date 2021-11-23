#include "tools.h"
#define RTT 100000
int main (int argc, char *argv[]) {
    FILE* file;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, nFrags, errors=0, toSend=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data, ack = 0;// les deux port UDP utilise
    char buffer_con[RCVSIZE], buffer_data[RCVSIZE], buffer_ack[10],buffer_file[RCVSIZE-6]; // le buffer_con ou on va recevoir des donnees et des connexion
    int udp_con, udp_data, n=0;
    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len);
    struct timeval tv = setTimer(0,10000);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));
    while(1){
        printf("[INFO] Waiting for message being name of file to send ...\n");    //verifyContents(buffer_data, buffer_file, 0, 20);
        n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        file = verifyFile(buffer_data, sizeof(buffer_data));
        size_t length = getLengthFile(file);
        nFrags = getNumberFragments(length, pLastFrag);
        for(int seqNumber=1;seqNumber < nFrags+1;seqNumber++){
            toSend=fread(buffer_file, 1, (seqNumber==nFrags)?(lastFragSize):(RCVSIZE-6), file);
            sprintf(buffer_data, "%6d", seqNumber);
            memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
            printf("[INFO] Sending file %d/%d ...\r", seqNumber, nFrags);
            ack = 0;
            while(ack == 0){
                sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack), MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);   
                if(strstr(buffer_ack, "ACK") != NULL) {
                    if(atoi(strtok(buffer_ack,"ACK")) == seqNumber){
                        ack = 1;
                        printf("[OK] Received ACK %d/%d\r",seqNumber, nFrags);}
                }else{
                    errors+=1;}}
            memset(buffer_data, 0, RCVSIZE);
            memset(buffer_ack, 0, sizeof(buffer_ack));
        }
        printf("\n");
        sendto(udp_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
        printf("[INFO] Waiting for message FINAL ACK to end connection ...\n");
        n = recvfrom(udp_data, (char *)buffer_ack, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);
        if(strstr(buffer_ack, "ACK") != NULL) {
            printf("[OK] Received FINAL ACK\n");
            memset(buffer_ack, 0, sizeof(buffer_ack));}
        fclose(file);
        printf("there have been %d errors in %d messages\n", errors, nFrags);
    } 
    return 0;
}