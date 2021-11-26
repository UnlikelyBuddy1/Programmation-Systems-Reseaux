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
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define RCVSIZE 1500

double give_time(void);
void envoyer(int no_seq, int no_bytes, char* buffer_input, char* buffer_output, int server_socket, struct sockaddr_in* client_addr, socklen_t length);
int wait_ack(int no_seq, int no_bytes, char* buffer_input, char* buffer_output, int server_socket, struct sockaddr_in* client_addr, socklen_t* length, int* previous,int* nb_same);
void *thread_clock(void* arguments);


struct arg_struct {
    sem_t* arg1;
    int* arg2;
    int* arg3;
    double* arg4;
};

int main (int argc, char *argv[]) {

    struct timeval timeout={5,0};
    struct sockaddr_in adresse, cliaddr, adresse2;
    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(&adresse, 0, sizeof(adresse));
    memset(&adresse2, 0, sizeof(adresse));
    pid_t ppid = getpid();

    int port_syn,port_data=1000;

    char buffer[RCVSIZE];

    if(argc > 2){
        //printf("Too many arguments\n");
        exit(0);
    }
    if(argc < 2){
        //printf("Argument expected\n");
        exit(0);
    }
    if(argc == 2){
        port_syn = atoi(argv[1]);
        
   
        
    }

    int server_desc_syn = socket(AF_INET, SOCK_DGRAM, 0);
    int server_desc_data = socket(AF_INET, SOCK_DGRAM, 0);

    if(server_desc_syn < 0){
        perror("Cannot create udp socket\n");
        return -1;
    }

    adresse.sin_family= AF_INET;
    adresse.sin_port= htons(port_syn);
    adresse.sin_addr.s_addr= INADDR_ANY;

    int newport = 1;
    port_data =  port_syn+newport;
    adresse2.sin_family= AF_INET;
    adresse2.sin_port= htons(port_data);
    adresse2.sin_addr.s_addr= INADDR_ANY;

    if (bind(server_desc_syn, (struct sockaddr*) &adresse, sizeof(adresse))<0) {
        perror("Bind failed\n");
        close(server_desc_syn);
        return -1;
    }

    socklen_t len = sizeof(cliaddr);
    int n;

    while(getpid() == ppid){
        //printf("JE SUIS LE PERE - %d\n",getpid());
        //printf("WAITING FOR SYN\n");
        n = recvfrom(server_desc_syn, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';

        if(strcmp(buffer,"SYN")==0){

            char synack[13];
            char port_data_s[6];
            strcpy(synack, "SYN-ACK");
            //printf("SYN COMING\n");
            snprintf((char *) port_data_s, 10 , "%d", port_data );
            strcat(synack,port_data_s);
            strcpy(buffer, synack);

            sendto(server_desc_syn, (char *)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
            //printf("Waiting for ACK...\n");
            n = recvfrom(server_desc_syn, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
            buffer[n] = '\0';
            if(strcmp(buffer,"ACK")==0){
                //printf("ACK received, connection\n");
            // Handshake reussi !
            } else{
                //printf("Bad ack");
                exit(0);
            }
        } else{
            //printf("bad SYN");
            exit(0);
        }

        if (bind(server_desc_data, (struct sockaddr*) &adresse2, sizeof(adresse2))<0) {
            
            perror("Bind failed\n");
            exit(0);
        }

        pid_t testf = fork();

        if(testf == 0){

            close(server_desc_syn);

        } else{

            close(server_desc_data);
            server_desc_data = socket(AF_INET, SOCK_DGRAM, 0);
            port_data ++;
            adresse2.sin_port= htons(port_data);
            
        }
    }
    FILE* fichier;

    timeout.tv_sec=0;
    timeout.tv_usec=50000;
    
    setsockopt(server_desc_data,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    n = recvfrom(server_desc_data, (char *)buffer, RCVSIZE,MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
    buffer[n] = '\0';

    fichier = NULL;

    if((fichier=fopen(buffer,"r"))==NULL){
        printf("Something's wrong I can feel it (file)\n");
        exit(1);
    }
    int small_size;
    size_t pos = ftell(fichier);    // Current position
    fseek(fichier, 0, SEEK_END);    // Go to end
    size_t length = ftell(fichier); // read the position which is the size
    fseek(fichier, pos, SEEK_SET);

    char* buffer_lecture = (char*)malloc(length * sizeof(char));
    
    if((fread(buffer_lecture,sizeof(char),length,fichier))!=length){
        printf("Something's wrong I can feel it (read)....\n");
    }
    fclose(fichier);

    int nb_morceaux;
    int reste = length % (RCVSIZE-6);

    if((length % (RCVSIZE-6)) != 0){
        nb_morceaux = (length/(RCVSIZE-6))+1;
    } else {
        nb_morceaux = length/(RCVSIZE-6);
    }
    if(length < 10000000){
        small_size = 1;
    }

    // --- Differents parametres de la gestion de la taille de la fenetre
    int num_seq=1;
    int num_seq_ack=0;
    int taille_window=1;
    int window[4096];
    int threshold=4096;
    
    
    // --- Differentes varaibles qui servent a l'estimation du RTT
    double time_rtt;
    int numseq_rtt;
    double time_in_us;
    double avg_rtt=0;

    // --- Varaible servant au calcul du debit final
    double time = give_time();
    
    // --- Statistiques
    int nb_timeout=0;
    int nb_retransmit=0;
    
    // --- Sert au fast retransmit (Combien de ACKS d'affilee sont les memes)
    int previous_ack;
    int nb_same_ack;

    // --- Sert au fast recovery (On force la CA après fast retransmit)
    int forced_cavoidance=0;
    
    // --- Materiel necessaire a la CA (thread ET semaphore)
    pthread_t thread1;
    sem_t semaphore1;
    sem_init(&semaphore1, 0, 1);
    
    // --- Structure et arguments pour le thread CA + variable lancement thread
    struct arg_struct args;
    args.arg1=&semaphore1;
    args.arg2=&taille_window;
    args.arg3=&threshold;
    args.arg4=&time_in_us;
    int first_time=1;
    
    //printf("Initialisation\n");
    for(int i=0;i< taille_window;i++){
        if(i==0){
            //printf("On lance le RTT calculation\n");
            time_rtt = give_time();
            numseq_rtt = i+1;
        }
        if(num_seq == nb_morceaux){
            envoyer(num_seq+i,reste,(char*)buffer_lecture,(char *)&buffer,server_desc_data,&cliaddr,len);
        } else {
            envoyer(num_seq+i,RCVSIZE-6,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,len);
        }
        window[i]=i+1;
    }

    num_seq=taille_window;

    while(num_seq_ack < nb_morceaux){
        
        int n = wait_ack(num_seq_ack,RCVSIZE-6,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,&len,&previous_ack,&nb_same_ack);
        sem_wait(&semaphore1);
        int taille_window_copy=taille_window;
        sem_post(&semaphore1);
        ////printf("Rentre dans le affichage de timer ? %d\n",(numseq_rtt <= n)&&(n!=0)&&(numseq_rtt != 0));
        if((n > (numseq_rtt+taille_window_copy))&&(n!=0)){
            numseq_rtt=0;
        }
        if((numseq_rtt == n)&&(n!=0)&&(numseq_rtt != 0)){
            
            time_rtt = give_time() - time_rtt;
            //printf("Hard RTT time is : %f\n",time_rtt);
            if((time_rtt < 1000000)){
                sem_wait(&semaphore1);
                time_in_us = time_rtt*1000000;
                if(avg_rtt != 0){
                    avg_rtt= (0.8*avg_rtt)+(0.2*time_in_us);
                } else {
                    avg_rtt+=time_in_us;
                }
                time_in_us=2*avg_rtt;
                timeout.tv_usec=time_in_us;
                sem_post(&semaphore1);
                setsockopt(server_desc_data,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
                
            }
            numseq_rtt=0;
        }

        if((n == 0) || (n==-1)){
            if((n==0)&&(small_size != 1)){
                nb_timeout++;
                sem_wait(&semaphore1);
                taille_window=1;
                threshold=(num_seq-num_seq_ack)/2;
                sem_post(&semaphore1);
            } else if((n==-1)&&(small_size!=1)){
                nb_retransmit++;
                sem_wait(&semaphore1);
                threshold=taille_window/2;
                taille_window=threshold+3;
                sem_post(&semaphore1);
                forced_cavoidance=1;
            } else if(small_size == 1){
                // Do nothing
            }

            //Recaler la window
            num_seq = num_seq_ack;
            sem_wait(&semaphore1);
            int taille_window_copy=1;
            sem_post(&semaphore1);
            
            for(int i=0;i< taille_window_copy;i++){
                if(i==0){
                    ////printf("On lance le RTT calculation\n");
                    time_rtt = give_time();
                    numseq_rtt = num_seq;
                }
                
                if((num_seq+i == nb_morceaux)&&(num_seq_ack <= nb_morceaux)){
                    envoyer(num_seq+i+1,reste,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,len);
                } else if((num_seq+i < nb_morceaux)&&(num_seq_ack <= nb_morceaux)){
                    envoyer(num_seq+i+1,RCVSIZE-6,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,len);
                }
                window[i]=num_seq+i+1;

            }
            num_seq += taille_window_copy;
            
        } else {
            sem_wait(&semaphore1);
            if((taille_window >= threshold)||(forced_cavoidance==1)){
                //printf("On slow down mofow\n");
                if(first_time==1){
                    
                    pthread_create(&thread1, NULL, thread_clock, (void *)&args);
                    first_time=0;
                }
                
                taille_window+= 1.0/taille_window;
                //TODO Each RTT increase taille_window par 1
            } else {
                if(num_seq_ack==0){
                    
                    taille_window+=1;
                } else {
                    
                    if((n-num_seq_ack)>=1){
                        taille_window+=n-num_seq_ack;
                    } 
                }
            }
            int taille_window_copy=taille_window;
            sem_post(&semaphore1);
            if(n > num_seq_ack){
                num_seq_ack = n;
            }
            
            for(int j=0;j<taille_window_copy;j++){

                if(window[j] < (num_seq_ack+1+j)){

                    window[j]=num_seq_ack+(j+1);
                    if(window[j] > num_seq){
                        
                        num_seq=window[j];
                        if((num_seq == nb_morceaux)&&(num_seq_ack <= nb_morceaux)){
                            envoyer(num_seq,reste,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,len);
                        } else if((num_seq < nb_morceaux)&&(num_seq_ack <= nb_morceaux)){
                            if((rand() % 3) == 1){
                                ////printf("On lance le RTT calculation\n");
                                time_rtt = give_time();
                                numseq_rtt=num_seq;
                            }
                            envoyer(num_seq,RCVSIZE-6,(char*)buffer_lecture,(char*)&buffer,server_desc_data,&cliaddr,len);
                        }
                    }
                }
            }
        }
    }

    // ToDo retransmission si pas ack
    for(int i=0;i<50;i++){
        sendto(server_desc_data,"FIN", strlen("FIN"),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,len);
    }
    
    n = recvfrom(server_desc_data, (char *)buffer, RCVSIZE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);
    buffer[n] = '\0';
    //printf("Final ack done for %d .\n",getpid());
    double time_taken = give_time() - time;
    printf("time taken %.6lf\n",time_taken);
    //printf("Taille: %ld\n", length);
    //printf("Temps: %f\n",time_taken);
    printf("%f\n",((float)((float)length/time_taken))/1000);
    printf("Nb de timeouts %d\n",nb_timeout);
    printf("Nb of retransmits %d\n",nb_retransmit);
    free(buffer_lecture);
    return 0;
}


void envoyer(int no_seq, int no_bytes, char* buffer_input, char* buffer_output, int server_socket, struct sockaddr_in* client_addr, socklen_t length){

    char num_seq_tot[7];
    char num_seq_s[7];

    memcpy(buffer_output+6, buffer_input+((RCVSIZE-6)*(no_seq-1)), no_bytes);
    strcpy(num_seq_tot, "000000");
    snprintf((char *) num_seq_s, 10 , "%d", no_seq );
    for(int i = strlen(num_seq_s);i>=0;i--){
        num_seq_tot[strlen(num_seq_tot)-i]=num_seq_s[strlen(num_seq_s)-i];
    }
    memcpy(buffer_output,num_seq_tot, 6);
    sendto(server_socket,(const char*)buffer_output, no_bytes+6 ,MSG_CONFIRM, (struct sockaddr*) client_addr,length);

    return;
}

int wait_ack(int no_seq, int no_bytes, char* buffer_input, char* buffer_output, int server_socket, struct sockaddr_in* client_addr, socklen_t* length, int* previous,int* nb_same){

    int n = recvfrom(server_socket, (char *)buffer_input, RCVSIZE,MSG_WAITALL, (struct sockaddr *) client_addr,length);
    buffer_input[n] = '\0';
    if(n != -1){
        
        int numack = atoi(strtok(buffer_input,"ACK_"));
        
        if(numack == *previous){
            (*nb_same)++;
            if((*nb_same)==3){
                (*nb_same)=0;
                return -1; //Mettre un identifier différent afin de reconnaitre pour fast retransmit
            }
        } else {
            (*nb_same)=0;
        }
        
        *previous=numack;
        if(numack == no_seq){
            //printf("Ack == Numero de seq\n");
            return numack;

        }
        else if((numack > no_seq)){
            //printf("Ack plus grand que le num de sequence : %d\n",numack);
            return numack;
        }
        else if(numack < no_seq){
            //printf("Already acknowledged !\n");
            return numack;
        }
    }

    return 0;
}

double give_time(void)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec*1e-9;
}

void *thread_clock(void* arguments) {
    
    struct arg_struct *args = arguments;
    int nb_ajouts=0;
    while(1){
        sem_wait(args->arg1);
        int* s_win = (args->arg2);
        int thres=*(args->arg3);
        double sleep_time=*(args->arg4);
        if(*s_win >= thres){
            *s_win+=1;
        }
        sem_post(args->arg1);
        usleep(sleep_time);
        nb_ajouts++;
        
    }
	pthread_exit(EXIT_SUCCESS);
}