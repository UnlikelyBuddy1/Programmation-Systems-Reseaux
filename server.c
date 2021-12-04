#include "tools.h"
int main (int argc, char *argv[]) {
    FILE* file;
    FILE* logs = fopen("log.txt", "w");
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, errors=0, toSend=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data;
    char buffer_con[MTU], buffer_data[MTU], buffer_ack[10],buffer_file[MTU-6];
    int udp_con, udp_data, n=0;

    unsigned int nFrags, micro_sec;
    struct timespec begin, end, waitACK, endACK;

    unsigned short iw = 8, cwnd= iw, ssthresh=20, lw = 8, duplicateACK = 0;
    unsigned long seqNum=1, ACKnum=0, windowPos=1, lastACK=0, RTT, RTO, SRTT;
    signed short flightsize = 0;
    float fcwnd;

    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    RTT=(TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len));
    printf("[INFO] RTT is %lu µs\n", RTT);
    RTO = 20000;
    struct timeval tv = setTimer(0,RTO);

    printf("[INFO] Waiting for message being name of file to send ...\n");
    n = recvfrom(udp_data, (char *)buffer_data, MTU, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));    
    file = verifyFile(buffer_data, sizeof(buffer_data));
    size_t length = getLengthFile(file);
    nFrags = getNumberFragments(length, pLastFrag);
    clock_gettime(CLOCK_REALTIME, &begin);
    fseek(file, 0, SEEK_SET);
    while(seqNum<=nFrags){
        memset(&waitACK, 0, sizeof(waitACK));
        clock_gettime(CLOCK_REALTIME, &waitACK);
        do {
            for(unsigned char i= 0; i<=cwnd-flightsize; i++){
                if(seqNum <= nFrags){
                    fseek(file, (seqNum-1)*(MTU-6), SEEK_SET);
                    toSend=fread(buffer_file, 1, (seqNum==nFrags)?(lastFragSize):(MTU-6), file);
                    sprintf(buffer_data, "%6ld", seqNum);
                    memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
                    printf("[INFO] Sending file %lu/%u ...\r", seqNum, nFrags);
                    sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                    flightsize++;
                    seqNum++;
                }
            }
            (LOG) ? printf("ACKnum: %lu\tduplicateACK: %u\tlastACK: %lu\tcwnd: %u\nfcwnd %f\tWindowPos %lu\tseqNum: %lu\t flightsize %d\n", ACKnum, duplicateACK, lastACK, cwnd, fcwnd, windowPos, seqNum, flightsize): pass();
            n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack)-1, MSG_PEEK, (struct sockaddr *) &cliaddr,&len); 
            if(n<=9){
                n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack)-1, MSG_DONTWAIT, (struct sockaddr *) &cliaddr,&len);
                buffer_ack[9]='\0';
                if(strstr(buffer_ack, "ACK") != NULL) {
                    if(ACKnum<atoi(strtok(buffer_ack,"ACK"))){
                        ACKnum=atoi(strtok(buffer_ack,"ACK"));
                    }
                    flightsize=flightsize-((ACKnum+1)-windowPos);
                    if(flightsize<0){
                        flightsize=0;
                    }
                    if(ACKnum == lastACK){
                        duplicateACK++;
                    } else {
                        duplicateACK = 0;   
                        if(cwnd<ssthresh){
                            cwnd=cwnd+(ACKnum-lastACK);
                            fcwnd=(float)cwnd;
                        } else {
                            fcwnd = (float)(cwnd + 1/cwnd);
                            cwnd = (unsigned short)fcwnd;
                        }
                        lastACK = ACKnum;
                    }
                    if(ACKnum+1>windowPos){
                        windowPos = ACKnum+1;
                        for(unsigned char i= 0; i<=cwnd-flightsize; i++){
                            if(flightsize<cwnd && seqNum <= nFrags){
                                fseek(file, (seqNum-1)*(MTU-6), SEEK_SET);
                                toSend=fread(buffer_file, 1, (seqNum==nFrags)?(lastFragSize):(MTU-6), file);
                                sprintf(buffer_data, "%6ld", seqNum);
                                memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
                                printf("[INFO] Sending file %lu/%u ...\r", seqNum, nFrags);
                                sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                                flightsize++;
                                seqNum++;
                            }
                        }
                    }
                }
            }
            clock_gettime(CLOCK_REALTIME, &endACK);
            micro_sec= ((endACK.tv_sec*1e6) + (endACK.tv_nsec/1e3)) - ((waitACK.tv_sec*1e6) + (waitACK.tv_nsec/1e3));
            (LOG) ? printf("time is %u µs/%lu µs\n", micro_sec, RTO): pass();
        } 
        while(micro_sec< RTO && duplicateACK<3 && seqNum<=nFrags);
        duplicateACK=0;
        if(windowPos!=seqNum){
            seqNum=lastACK+1;
            flightsize=0;
            errors+=1;
            if(cwnd<(iw*2)){
                cwnd=lw;
            } else {
                cwnd=(unsigned short)((cwnd/2)+1);
            }
        }
        else{
            if(cwnd<ssthresh){
                cwnd=cwnd*2;
            }
            else{
                cwnd+=1;
            }
        }
        (LOG) ? fprintf(logs, "%d\n", cwnd) : (pass());
        memset(buffer_data, 0, MTU);
        memset(buffer_ack, 0, sizeof(buffer_ack));
    }
    printf("\n");
    sendto(udp_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("[INFO] Bandwith: %.3f Ko/s.\n", length/(elapsed*1000));
    fclose(file);
    printf("[INFO] There have been %d errors in %d messages\n", errors, nFrags);
    
    return 0;
}