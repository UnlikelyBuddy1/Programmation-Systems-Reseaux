#include "tools.h"
int main(int argc, char *argv[])
{
    unsigned short port_udp_con, *pport_udp_con = &port_udp_con, k = 0;
    int udp_con, udp_data, n = 0;
    char buffer_con[MTU];
    pid_t pid;

    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));

    verifyArguments(argc, argv, pport_udp_con);
    udp_con = createSocket();
    udp_con = bindSocket(udp_con, port_udp_con);
    printf("Mon PID est %i\n", getpid());
    while (1)
    {
        k++;
        printf("waiting for client connexions, creating socket on port %u\n", k);
        udp_data = createSocket();
        udp_data = bindSocket(udp_data, port_udp_con + k);
        TWH(udp_con, n, buffer_con, port_udp_con + k, cliaddr, len);
        printf("a client is connecting\n");

        pid = fork();
        if (pid == 0)
        {
            printf("[INFO] PID is %d\n", pid);
            FILE *file;
            unsigned short lastFragSize, *pLastFrag = &lastFragSize, toSend = 0;
            char buffer_data[MTU], buffer_ack[10], buffer_file[MTU - 6];
            unsigned char retransmit = 0, timeout = 0;
            unsigned short cwnd = 10, duplicateACK = 0, duplicateTrigger = 2;
            unsigned long seqNum = 1, ACKnum = 0, errors = 0, retransmits = 0, nFrags, ACK, RTO, sent = 0;
            struct timespec begin, end;

            RTO = 2500;

            printf("[INFO] Waiting for message being name of file to send ...\n");
            n = recvfrom(udp_data, (char *)buffer_data, MTU, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
            file = verifyFile(buffer_data, sizeof(buffer_data));
            size_t length = getLengthFile(file);
            nFrags = getNumberFragments(length, pLastFrag);
            clock_gettime(CLOCK_REALTIME, &begin);
            fseek(file, 0, SEEK_SET);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = RTO;

            fd_set socket;
            int retval;
            FD_ZERO(&socket);
            FD_SET(udp_data, &socket);

            signed long ACK_TIMERS[nFrags];

            while (ACKnum < nFrags)
            {
                do
                {
                    if (retransmit == 1)
                    {
                        fseek(file, (ACKnum) * (MTU - 6), SEEK_SET);
                        toSend = fread(buffer_file, 1, (ACKnum == nFrags) ? (lastFragSize) : (MTU - 6), file);
                        sprintf(buffer_data, "%6ld", ACKnum + 1);
                        memcpy(buffer_data + 6, buffer_file, sizeof(buffer_file));
                        (LOG) ? pass() : printf("[INFO] Sending file %lu/%lu ...\r", ACKnum + 1, nFrags);
                        sendto(udp_data, (const char *)buffer_data, toSend + 6, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                        retransmit = 0;
                        ACK_TIMERS[ACKnum] = RTO;
                        sent++;
                        (LOG) ? printf("[RETRANSMIT] ACKnum: %lu\tduplicateACK: %u\tseqNum: %lu\n", ACKnum, duplicateACK, seqNum) : pass();
                    }
                    else
                    {
                        (LOG) ? printf("[SENDING] %ld < %u\t: ", (seqNum - (ACKnum + 1)), cwnd) : pass();
                        while ((seqNum - (ACKnum + 1)) < cwnd)
                        {
                            if (seqNum <= nFrags)
                            {
                                fseek(file, (seqNum - 1) * (MTU - 6), SEEK_SET);
                                toSend = fread(buffer_file, 1, (seqNum == nFrags) ? (lastFragSize) : (MTU - 6), file);
                                sprintf(buffer_data, "%6ld", seqNum);
                                memcpy(buffer_data + 6, buffer_file, sizeof(buffer_file));
                                (LOG) ? pass() : printf("[INFO] Sending file %lu/%lu ...\r", seqNum, nFrags);
                                sendto(udp_data, (const char *)buffer_data, toSend + 6, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                                (LOG) ? printf("%lu, ", seqNum) : pass();
                                ACK_TIMERS[seqNum] = RTO;
                                seqNum++;
                                sent++;
                            }
                            else
                            {
                                break;
                            }
                        }
                        (LOG) ? printf("\n") : pass();
                    }

                    retval = select(10, &socket, NULL, NULL, &tv);
                    if (retval == 0)
                    {
                    }
                    else
                    {
                        n = recvfrom(udp_data, (char *)buffer_ack, sizeof(buffer_ack) - 1, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
                        if (n >= 9)
                        {
                            buffer_ack[10] = '\0';
                            if (strstr(buffer_ack, "ACK") != NULL)
                            {
                                ACK = atoi(strtok(buffer_ack, "ACK"));
                                if (ACKnum < ACK)
                                {
                                    ACKnum = ACK;
                                    duplicateACK = 0;
                                    (LOG) ? printf("[ACK] RCV : %lu\n", ACK) : pass();
                                }
                                else if (ACK == ACKnum)
                                {
                                    duplicateACK++;
                                    (LOG) ? printf("[DUPLICATE] RCV : %lu CURRENT : %lu\n", ACK, ACKnum) : pass();
                                }
                                if (ACKnum + 1 > seqNum)
                                {
                                    seqNum = ACKnum + 1;
                                }
                            }
                            memset(buffer_ack, 0, sizeof(buffer_ack));
                        }
                    }
                    FD_ZERO(&socket);
                    FD_SET(udp_data, &socket);

                    for (unsigned long i = seqNum - cwnd; i < seqNum; i++)
                    {
                        if (i >= ACKnum)
                        {
                            ACK_TIMERS[i] = (tv.tv_sec * 1e6 + tv.tv_usec);
                        }
                        if (ACK_TIMERS[i] <= 0)
                        {
                            timeout = 1;
                        }
                    }
                    tv.tv_usec = RTO;
                    tv.tv_sec = 0;
                } while (timeout == 0 && (duplicateACK % (cwnd - duplicateTrigger)) && duplicateACK != duplicateTrigger && seqNum <= nFrags);
                if (duplicateACK % (cwnd - duplicateTrigger) || duplicateACK == duplicateTrigger)
                {
                    retransmits++;
                    retransmit = 1;
                }
                if (timeout != 0)
                {
                    (LOG) ? printf("[TIMEOUT]\n") : pass();
                    errors++;
                    retransmit = 0;
                    seqNum = ACKnum + 1;
                    timeout = 0;
                }
                memset(buffer_data, 0, MTU);
                memset(buffer_ack, 0, sizeof(buffer_ack));
                (LOG) ? printf("[INFO] ACKnum is %lu and nFrags is %lu\n", ACKnum, nFrags): pass();
            }
            sendto(udp_data, "FIN", strlen("FIN"), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
            clock_gettime(CLOCK_REALTIME, &end);
            long seconds = end.tv_sec - begin.tv_sec;
            long nanoseconds = end.tv_nsec - begin.tv_nsec;
            double elapsed = seconds + nanoseconds * 1e-9;
            printf("[INFO] Bandwith: %.3f Ko/s. Took %.4f seconds\n", length / (elapsed * 1000), seconds + (nanoseconds / 1e9));
            fclose(file);
            printf("[INFO] There have been %lu timeouts and %lu retransmits in %lu messages, total of %lu sent\n", errors, retransmits, nFrags, sent);
            close(udp_data);
            exit(0);
        } else {
            close(udp_data);
        }
    }
    return 0;
}