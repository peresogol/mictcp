/*


Pb ack / seq num : séquence Abandon abandon abandon retry (n fois éventuellement)
Dans le cas d'une perte

*/

#include <mictcp.h>
#include <api/mictcp_core.h>

#define LOSS_RATE 1
#define LOSS_WINDOW_SIZE 10
#define LOSS_ACCEPTANCE 30
#define MAX_TRY_CONNECT 15
#define TIMER 5

protocol_state client_state;
protocol_state serveur_state;

//sockets
mic_tcp_sock my_socket;
mic_tcp_sock_addr sock_addr;
int num_socket = 10; //pour generer un identifiant unique propre au socket

//numéros de séquence
int seq_num = 0;
int ack_num = 0;

//reprise des pertes
int nb_send = 0;
int effective_loss_rate = 0;

/////////////////////
int loss_window[LOSS_WINDOW_SIZE]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int loss_window_index = 0;


/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm) {
    
   int result;

   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(LOSS_RATE);

    if (result==-1){
        printf("Erreur dans l'initialisation des components\n");
        exit(1);
    } else {
        my_socket.fd = num_socket; //identifiant du socket, doit etre unique
        my_socket.state = IDLE;
        result = my_socket.fd;
        num_socket++;
    }

    return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr) {
 
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) {
    
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    //serveur_state = ACCEPT;

    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) {
    
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   /* 
    mic_tcp_pdu pdu_send;
    mic_tcp_pdu pdu_recv;
    int res_send;
    int res_recv;
    int nb_try = 0;

    if (my_socket.fd != mic_sock){
        return -1;
    }

    pdu_send.header.source_port = my_socket.addr.port; 
    pdu_send.header.dest_port = my_socket.addr_dist.port; 
    pdu_send.header.syn = 1;n abandonne
    do {
        res_send = IP_send(pdu_send, my_socket.addr_dist); 
        res_recv = IP_recv(&pdu_recv, NULL, 100);

    } while (res_recv == -1 && nb_try < MAX_TRY_CONNECT);
      
    client_state = SYN_SENT;

    // envoie ACK dans process pdu ?
*/
    return 0;
}



int calcul_loss_rate(){
    int loss=0;
    for (int i = 0; i<LOSS_WINDOW_SIZE; i++){ //calcul du loss rate
        loss += lossWindow[i];
    }
    return loss * 100 / LOSS_WINDOW_SIZE;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size) {
    
    mic_tcp_pdu pdu_send;
    mic_tcp_pdu pdu_recv = {0};
    int res_send;
    int res_recv;
    int retry = 0;

    //printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    if (my_socket.fd != mic_sock){
        return -1;
    }

    pdu_send.header.source_port = my_socket.addr.port; 
    pdu_send.header.dest_port = my_socket.addr_dist.port; 
    pdu_send.header.seq_num = seq_num; 

    pdu_send.payload.size = mesg_size;
    pdu_send.payload.data = malloc (sizeof(char)*mesg_size);
    memcpy(pdu_send.payload.data, mesg, mesg_size);
   
    nb_send++;
    
    do {

        if (loss_window_index == LOSS_WINDOW_SIZE) { //cyclage du tableau de pertes
            loss_window_index = 0;
        }

        res_send = IP_send(pdu_send, my_socket.addr_dist); 
        res_recv = IP_recv(&pdu_recv, NULL, TIMER);

        
        printf("ENVOI %d ", nb_send);
        if(res_recv == -1 || pdu_recv.header.ack_num != seq_num) { //echec de reception de l'acquittement ou mauvais numero d'acquittement reçu
            printf("ECHEC : ");
            loss_window[loss_window_index] = 1;
            effective_loss_rate = calcul_loss_rate();
            
            // Cas trop de pertes, on renvoie
            if(effective_loss_rate > LOSS_ACCEPTANCE){ 
                printf("RETRY\n");                
                retry = 1;
            } else {
                printf("ABANDON\n");
                retry = 0;
            }    
        } else {  // succès envoie
            printf("SUCCES\n");
            loss_window[loss_window_index] = 0;
            retry = 0;
        }

        loss_window_index++;

        printf("taux de pertes : %d\n", calcul_loss_rate());
    } while (retry);
    
    seq_num = (seq_num + 1) % 2;

    return res_send;
}


/*
 * Calcule le taux de pertes sur la fenêtre glissante 
 *  Renvoie un entier correspondant à ce taux (en %)
 */
int calcul_loss_rate(){
    int loss = 0;
    for (int i = 0; i < LOSS_WINDOW_SIZE; i++){ //calcul du loss rate
        loss += loss_window[i];
    }
    return loss * 100 / LOSS_WINDOW_SIZE;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size) {   
     
    mic_tcp_pdu pdu;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    pdu.payload.data = mesg;
    pdu.payload.size = max_mesg_size;

    return app_buffer_get(pdu.payload);
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr) {
    
    mic_tcp_pdu pdu_ack;

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    if(pdu.header.seq_num == ack_num) {
        app_buffer_put(pdu.payload);
    } 

    pdu_ack.header.source_port = my_socket.addr.port;
    pdu_ack.header.dest_port = my_socket.addr_dist.port;
    pdu_ack.header.ack_num = ack_num;

    IP_send(pdu_ack, my_socket.addr_dist);
    ack_num = (ack_num + 1 ) % 2;

}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket) {

    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 0;
}
