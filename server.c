#include "tools.h"
int main (int argc, char *argv[]) {
    FILE* file;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));
    unsigned short port_udp_con, port_udp_data, lastFragSize, *pLastFrag=&lastFragSize, toSend=0;
    unsigned short *pport_udp_con= &port_udp_con, *pport_udp_data= &port_udp_data;
    char buffer_con[MTU], buffer_data[MTU], buffer_ack[10],buffer_file[MTU-6];
    int udp_con, udp_data, n=0;

    unsigned int micro_sec;
    struct timespec begin, end, waitACK, endACK;

    unsigned short cwnd= 10, duplicateACK = 0, retransmit=0, duplicateTrigger = 2;
    unsigned long seqNum=1, ACKnum=0, lastACK=0, RTT, RTO, SRTT, errors=0, retransmits = 0, nFrags, ACK;

    verifyArguments(argc, argv, pport_udp_con, pport_udp_data);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    udp_data = createSocket();
    udp_data = bindSocket(udp_data, port_udp_data);
    RTT=(TWH(udp_con, n, buffer_con, port_udp_data, cliaddr, len));
    printf("[INFO] RTT is %lu µs\n", RTT);
    RTO = 6125;
    struct timeval tv = setTimer(0,RTO);

    printf("[INFO] Waiting for message being name of file to send ...\n");
    n = recvfrom(udp_data, (char *)buffer_data, MTU, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv));    
    file = verifyFile(buffer_data, sizeof(buffer_data));
    size_t length = getLengthFile(file);
    nFrags = getNumberFragments(length, pLastFrag);
    clock_gettime(CLOCK_REALTIME, &begin);
    fseek(file, 0, SEEK_SET);
    memset(&waitACK, 0, sizeof(waitACK));
    clock_gettime(CLOCK_REALTIME, &waitACK);
    setsockopt(udp_data, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    while(ACKnum<nFrags){
        do {
            if(retransmit==1){
                fseek(file, (ACKnum)*(MTU-6), SEEK_SET);
                toSend=fread(buffer_file, 1, (ACKnum==nFrags)?(lastFragSize):(MTU-6), file);
                sprintf(buffer_data, "%6ld", ACKnum+1);
                memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
                (LOG) ? pass(): printf("[INFO] Sending file %lu/%lu ...\r", ACKnum+1, nFrags);
                sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                retransmit=0;
                (LOG) ? printf("[RETRANSMIT] ACKnum: %lu\tduplicateACK: %u\tseqNum: %lu\n", ACKnum, duplicateACK, seqNum): pass();
            } else {
                (LOG) ? printf("[SENDING] %ld < %u\t:", (seqNum-(ACKnum+1)), cwnd): pass();
                while((seqNum-(ACKnum+1))<cwnd){
                    if(seqNum <= nFrags){
                        fseek(file, (seqNum-1)*(MTU-6), SEEK_SET);
                        toSend=fread(buffer_file, 1, (seqNum==nFrags)?(lastFragSize):(MTU-6), file);
                        sprintf(buffer_data, "%6ld", seqNum);
                        memcpy(buffer_data+6, buffer_file, sizeof(buffer_file));
                        (LOG) ? pass(): printf("[INFO] Sending file %lu/%lu ...\r", seqNum, nFrags);
                        sendto(udp_data,(const char*)buffer_data, toSend+6, MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
                        (LOG) ? printf(" %lu \t", seqNum): pass();
                        seqNum++;
                    } else {
                        break;
                    }
                }
                (LOG) ? printf("\n"): pass();
            }
            
            n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack), MSG_PEEK, (struct sockaddr *) &cliaddr,&len); 
            if(n>=9){
                n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack)-1, MSG_DONTWAIT, (struct sockaddr *) &cliaddr,&len);
                buffer_ack[10]='\0';
                if(strstr(buffer_ack, "ACK") != NULL) {
                    ACK=atoi(strtok(buffer_ack,"ACK"));
                    if(ACKnum<ACK){
                        ACKnum=ACK;
                        duplicateACK=0;
                        (LOG) ? printf("[ACK] RCV : %lu\n", ACK): pass();
                    }
                    else if(ACK == ACKnum){
                        duplicateACK++;
                        (LOG) ? printf("[DUPLICATE] RCV : %lu CURRENT : %lu\n", ACK, ACKnum): pass();
                    }
                    if(ACKnum+1>seqNum){
                        seqNum = ACKnum+1;
                    }
                    clock_gettime(CLOCK_REALTIME, &waitACK);
                }
                memset(buffer_ack, 0, sizeof(buffer_ack));
                //(LOG) ? printf("[RCV ACK] ACKnum: %lu\tduplicateACK: %u\tlastACK: %lu\tcwnd: %u\tseqNum: %lu\n", ACKnum, duplicateACK, lastACK, cwnd, seqNum): pass();
                (LOG) ? printf("[TIMER] time is %u µs/%lu µs\n", micro_sec, RTO): pass();
            }
            clock_gettime(CLOCK_REALTIME, &endACK);

            micro_sec= ((endACK.tv_sec*1e6) + (endACK.tv_nsec/1e3)) - ((waitACK.tv_sec*1e6) + (waitACK.tv_nsec/1e3));
        } 
        while(micro_sec< RTO && (duplicateACK%(cwnd-duplicateTrigger)) && duplicateACK!=duplicateTrigger && seqNum<=nFrags);
        if(duplicateACK%(cwnd-duplicateTrigger) || duplicateACK==duplicateTrigger){
            retransmits++;
            retransmit=1;
        } 
        if(micro_sec> RTO){
            (LOG) ? printf("[TIMEOUT] time is %u µs/%lu µs\n", micro_sec, RTO): pass();
            errors++;
            retransmit=0;
            seqNum=ACKnum+1;
        }  
        memset(buffer_data, 0, MTU);
        memset(buffer_ack, 0, sizeof(buffer_ack));
        (LOG) ? printf("[INFO] ACKnum is %lu and nFrags is %lu\n", ACKnum, nFrags): pass();
    }
    sendto(udp_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    printf("[INFO] Bandwith: %.3f Ko/s. Took %.4f seconds\n", length/(elapsed*1000), seconds+(nanoseconds/1e9));
    fclose(file);
    printf("[INFO] There have been %lu timeouts and %lu retransmits in %lu messages\n", errors, retransmits, nFrags);
    
    return 0;
}