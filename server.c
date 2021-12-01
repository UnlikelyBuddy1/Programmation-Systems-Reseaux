#include "tools.h"
int main (int argc, char *argv[]) {
    FILE* file;
    FILE* logs = fopen("log.txt", "w");
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, errors=0, toSend=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data, window=10;
    char buffer_con[RCVSIZE], buffer_data[RCVSIZE], buffer_ack[10],buffer_file[RCVSIZE-6];
    int udp_con, udp_data, n=0;
    unsigned int windowPos=1, seqNumber= 1, nFrags, micro_sec, RTT;
    clock_t before;

    struct timespec begin, end; 

    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    RTT=(TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len));
    printf("[INFO] RTT is %d µs\n", RTT);
    RTT= 500;
    struct timeval tv = setTimer(0,RTT);

    printf("[INFO] Waiting for message being name of file to send ...\n");
    n = recvfrom(udp_data, (char *)buffer_data, RCVSIZE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));    
    file = verifyFile(buffer_data, sizeof(buffer_data));
    size_t length = getLengthFile(file);
    nFrags = getNumberFragments(length, pLastFrag);

    clock_gettime(CLOCK_REALTIME, &begin);

    while(windowPos<=nFrags){
        seqNumber=windowPos;
        while(seqNumber < windowPos+window && seqNumber<=nFrags){
            toSend=fread(buffer_file, 1, (seqNumber==nFrags)?(lastFragSize):(RCVSIZE-6), file);
            sprintf(buffer_data, "%6d", seqNumber);
            memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
            printf("[INFO] Sending file %d/%d ...\r", seqNumber, nFrags);
            sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
            seqNumber++;
        }
        before = clock();
        do {
            n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack)-1, MSG_PEEK, (struct sockaddr *) &cliaddr,&len); 
            if(n<=9){
                n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack)-1, MSG_DONTWAIT, (struct sockaddr *) &cliaddr,&len);
                buffer_ack[9]='\0';
                if(strstr(buffer_ack, "ACK") != NULL) {
                    windowPos=atoi(strtok(buffer_ack,"ACK"))+1;
                    fseek(file, (windowPos-1)*(RCVSIZE-6), SEEK_SET);
                }
            }
            clock_t difference= clock()-before;
            micro_sec= ((difference*1000000)/CLOCKS_PER_SEC);
        } 
        while(micro_sec< RTT && windowPos != seqNumber);
        if(windowPos!=seqNumber){
            errors+=1;
            window=(unsigned short)((window/2)+1);
        }
        else{
            if(window<12){
                window=window*2;
            }
            else{
                window+=1;
            }
        }
        (LOG) ? fprintf(logs, "%d\n", window) : (pass());
        memset(buffer_data, 0, RCVSIZE);
        memset(buffer_ack, 0, sizeof(buffer_ack));
    }
    printf("\n");
    sendto(udp_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("[INFO] Time measured: %.3f seconds.\n", elapsed);

    fclose(file);
    printf("[INFO] There have been %d errors in %d messages\n", errors, nFrags);
    
    return 0;
}