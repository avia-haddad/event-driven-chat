#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>

#include "chat_server.h"

#define RECV_BUFFER_SIZE 4096

static int end_server = 0;

void intHandler(int SIG_INT) {
    /* use a flag to end_server to break the main loop */
    /* OK */
    end_server = 1;
}

static int set_non_blocking(int socket_fd, int block_on){
    int ret_val;

    /* set non blocking mode */
    ret_val = ioctl(socket_fd, FIONBIO, (char *)&block_on);
    if (ret_val < 0){
        perror("set_non_blocking ioctl() failed");
        /* close(listen_fd); */
        return -1;
    }

    return 0;

}


int create_listening_socket(int port){
    int listen_fd;
    struct sockaddr_in servaddr;
    int ret_val;


    /* create listening TCP socket */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1){
        perror("Listen socket create error");
        return -1;
    }

    set_non_blocking(listen_fd,1);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    /* binding server addr structure to listenfd */
    ret_val = bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    if(ret_val == -1){
        perror("Listen socket bind error");
        return -1;
    }

    ret_val = listen(listen_fd, 10);


    if(ret_val == -1){
        perror("Listen socket listen error");
        return -1;
    }

    return listen_fd;

}
static void cleanup_connections(conn_pool_t *pool){

    conn_t *conn_iterator = NULL;
    conn_t *conn_iterator_to_delete = NULL;

    conn_iterator = pool->conn_head;

    while(conn_iterator != NULL){
        close(conn_iterator->fd);

        conn_iterator_to_delete = conn_iterator;
        conn_iterator = conn_iterator->next;
        free(conn_iterator_to_delete);
    };


}
int main (int argc, char *argv[])
{
    int listen_fd;
    /*
    fd_set rset;
     int max_fd_for_select;
     int nready;
    */

    if (argc < 2 || argc > 2)
    {
        printf("Usage: chatServer <port>\n");
        exit(1);
    }
    struct sockaddr_in client_addr;
    socklen_t client_adr_len;
    int conn_fd;
    /* const char *welcome_str = "Hello \n"; */
    int conn_fd_index;
    char recv_buffer[RECV_BUFFER_SIZE];
    int read_ret_val;
    conn_pool_t* pool = NULL;

    signal(SIGINT, intHandler);

    pool = malloc(sizeof(conn_pool_t));
    init_pool(pool);

    /*************************************************************/
    /* Create an AF_INET stream socket to receive incoming      */
    /* connections on                                            */
    /*************************************************************/
    listen_fd = create_listening_socket(atoi(argv[1]));

    if(listen_fd == -1){
        fprintf(stderr, "Coudn't create listening socket\n");
        return -1;
    }


    /*************************************************************/
    /* Set socket to be nonblocking. All of the sockets for      */
    /* the incoming connections will also be nonblocking since   */
    /* they will inherit that state from the listening socket.   */
    /*************************************************************/
    /* OK imlemented in create_listening_socket */
    /*************************************************************/
    /* Bind the socket                                           */
    /*************************************************************/
    /* OK imlemented in create_listening_socket */

    /*************************************************************/
    /* Set the listen back log                                   */
    /*************************************************************/
    /* OK imlemented in create_listening_socket */

    /*************************************************************/
    /* Initialize fd_sets  			                             */
    /*************************************************************/
    /* clear the descriptor set */

    FD_SET(listen_fd, &pool->read_set);
    pool->maxfd = listen_fd + 1;
    /*************************************************************/
    /* Loop waiting for incoming connects, for incoming data or  */
    /* to write data, on any of the connected sockets.           */
    /*************************************************************/
    do{
        /**********************************************************/
        /* Copy the master fd_set over to the working fd_set.     */
        /**********************************************************/

        pool->ready_read_set = pool->read_set;
        pool->ready_write_set = pool->write_set;

        /**********************************************************/
        /* Call select() 										  */
        /**********************************************************/
        printf("Waiting on select()...\nMaxFd %d\n", pool->maxfd);
        pool->nready = select(pool->maxfd, &pool->ready_read_set , &pool->ready_write_set , NULL, NULL);

        /* printf("pool->nready %d \n",pool->nready); */

        if(pool->nready == -1){ /* from SIGINT probably*/
            perror("Select warning");
            end_server = 1;
            break;
        }


        /**********************************************************/
        /* One or more descriptors are readable or writable.      */
        /* Need to determine which ones they are.                 */
        /**********************************************************/

        for (conn_fd_index = 3 ; conn_fd_index < pool->maxfd ; conn_fd_index++){
            /* Each time a ready descriptor is found, one less has  */
            /* to be looked for.  This is being done so that we     */
            /* can stop looking at the working set once we have     */
            /* found all of the descriptors that were ready         */

            /*******************************************************/
            /* Check to see if this descriptor is ready for read   */
            /*******************************************************/
            if (FD_ISSET(conn_fd_index, &pool->ready_read_set)){
                /***************************************************/
                /* A descriptor was found that was readable		   */
                /* if this is the listening socket, accept one      */
                /* incoming connection that is queued up on the     */
                /*  listening socket before we loop back and call   */
                /* select again. 						            */
                /****************************************************/

                if(conn_fd_index == listen_fd){
                    client_adr_len = sizeof(client_addr);
                    conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_adr_len);
                    printf("New incoming connection on sd %d\n", conn_fd);

                    add_conn(conn_fd,pool);

                }else{

                    /****************************************************/
                    /* If this is not the listening socket, an             */
                    /* existing connection must be readable                */
                    /* Receive incoming data his socket             */
                    /****************************************************/
                    printf("Descriptor %d is readable\n", conn_fd_index);
                    read_ret_val = read(conn_fd_index, recv_buffer, sizeof(recv_buffer));

                    if(read_ret_val == -1){
                        perror("Receive error");
                        assert(0);
                    }

                    if(read_ret_val == 0){
                        /* printf("End of file reached on conn : %d\n",conn_fd_index); */
                        /* remove the connection (remove_conn(...))            */
                        /* OK */

                        printf("Connection closed for sd %d\n",conn_fd_index);
                        close(conn_fd_index);
                        remove_conn(conn_fd_index, pool);
                    }

                    /* If the connection has been closed by client         */
                    /* remove the connection (remove_conn(...))            */
                    /* OK */

                    /**********************************************/
                    /* Data was received, add msg to all other    */
                    /* connectios                                    */
                    /**********************************************/

                    if(read_ret_val > 0){
                        /* add_msg(...); */
                        /* OK */
                        printf("%d bytes received from sd %d\n", read_ret_val, conn_fd_index);
                        add_msg(conn_fd_index, recv_buffer,read_ret_val, pool);
                    }



                }




            } /* End of if (FD_ISSET()) */
            /*******************************************************/
            /* Check to see if this descriptor is ready for write  */
            /*******************************************************/
            if (FD_ISSET(conn_fd_index, &pool->ready_write_set)) {
                /* try to write all msgs in queue to sd */
                /* OK */
                write_to_client(conn_fd_index, pool);
            }
            /*******************************************************/


        } /* End of loop through selectable descriptors */

    } while (end_server == 0);

    /*************************************************************/
    /* If we are here, Control-C was typed,						 */
    /* clean up all open connections					         */
    /*************************************************************/

    set_non_blocking(listen_fd,0);
    cleanup_connections(pool);
    shutdown(listen_fd, SHUT_RDWR);
    close(listen_fd);
    free(pool);
    return 0;
}


int init_pool(conn_pool_t* pool) {

    assert(pool);

    memset(pool, 0, sizeof(conn_pool_t));

    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_read_set);
    FD_ZERO(&pool->write_set);
    FD_ZERO(&pool->ready_write_set);

    return 0;
}

int add_conn(int sd, conn_pool_t* pool) {
    conn_t *new_connection = NULL;
    conn_t *insert_pos = NULL;
    /*
     * 1. allocate connection and init fields
     * 2. add connection to pool
     * */

    // printf("Adding connection fd %d to pool\n",sd);

    new_connection = malloc(sizeof(conn_t));
    assert(new_connection);

    memset(new_connection,0,sizeof(conn_t));

    new_connection->fd = sd;


    if( pool->conn_head == NULL){
        pool->conn_head = new_connection;
        pool->nr_conns = 1;
    }else{
        insert_pos = pool->conn_head;

        while(insert_pos->next != NULL){
            insert_pos = insert_pos->next;
        }

        insert_pos->next = new_connection;
        new_connection->prev = insert_pos;

        pool->nr_conns++;
    }

    FD_SET(sd, &pool->read_set);
    /* FD_SET(sd, &pool->write_set);  moved to msg add*/

    if(sd + 1 > pool->maxfd){
        pool->maxfd = sd + 1;
    }


    //printf("Connection count in pool : %d\n",pool->nr_conns);


    return 0;
}


int remove_conn(int sd, conn_pool_t* pool) {
    conn_t *conn_iterator = NULL;
    conn_t *conn_iterator_prev = NULL;
    conn_t *conn_iterator_to_delete = NULL;

    /*
    * 1. remove connection from pool
    * 2. deallocate connection
    * 3. remove from sets
    * 4. update max_fd if needed
    */

    conn_iterator = pool->conn_head;

    while(conn_iterator != NULL){

        conn_iterator_to_delete = NULL;

        if(conn_iterator->fd == sd){

            printf("removing connection with sd %d \n",sd);

            if(conn_iterator_prev == NULL){ /* if it is the first conn */

                if(conn_iterator->next){ /* if there is more than one */
                    pool->conn_head = conn_iterator->next;
                    conn_iterator_to_delete = conn_iterator;
                    /* free(conn_iterator); */
                }else{
                    pool->conn_head = NULL;
                    conn_iterator_to_delete = conn_iterator;
                    /* free(conn_iterator); */

                }

            }else{ /* if it is middle conn */

                if(conn_iterator->next){ /* if there is next one */
                    conn_iterator_prev->next = conn_iterator->next;
                    conn_iterator_to_delete = conn_iterator;
                    /* free(conn_iterator); */
                }else{
                    /* if there no next one */
                    conn_iterator_prev->next = NULL;
                    conn_iterator_to_delete = conn_iterator;
                    /* free(conn_iterator); */

                }

            }

            pool->nr_conns--;
        } /* if(conn_iterator->fd == sd) */

        conn_iterator_prev = conn_iterator;
        conn_iterator = conn_iterator->next;

        if(conn_iterator_to_delete){
            free(conn_iterator_to_delete);
        }


    } /* while(conn_iterator != NULL) */

    FD_CLR(sd, &pool->read_set);
    FD_CLR(sd, &pool->write_set);



    if(sd + 1 == pool->maxfd){
        pool->maxfd--;
    }
    return 0;
}

int add_msg(int sd, char* buffer,int len, conn_pool_t* pool) {
    conn_t *conn_iterator = NULL;
    msg_t *msg_to_add = NULL;
    msg_t *msg_iterator = NULL;


    /*
     * 1. add msg_t to write queue of all other connections
     * 2. set each fd to check if ready to write
     */



    conn_iterator = pool->conn_head;

    while(conn_iterator != NULL){

        if(conn_iterator->fd != sd){ /* add to other message queue other than self */

            FD_SET(conn_iterator->fd , &pool->write_set); /* new loc for write fd_set */

            /* create new msg instance begin */
            msg_to_add = malloc(sizeof(msg_t));
            assert(msg_to_add);

            memset(msg_to_add,0,sizeof(msg_t));
            msg_to_add->size = len;

            msg_to_add->message = malloc(len);
            assert(msg_to_add->message);

            memcpy(msg_to_add->message, buffer, len);
            /* create new msg instance end */

            if(conn_iterator->write_msg_head){

                msg_iterator = conn_iterator->write_msg_head;

                while(msg_iterator->next != NULL){ /* search last msg */
                    msg_iterator = msg_iterator->next;
                }

                msg_iterator->next = msg_to_add;

            }else{

                conn_iterator->write_msg_head = msg_to_add;
            }
        }

        conn_iterator = conn_iterator->next;
    }
    return 0;
}

int write_to_client(int sd, conn_pool_t* pool) {

    conn_t *conn_iterator = NULL;
    msg_t *msg_iterator = NULL;
    msg_t *msg_iterator_to_delete = NULL;
    int write_ret_val = 0;

    /*
     * 1. write all msgs in queue
     * 2. deallocate each writen msg
     * 3. if all msgs were writen successfully, there is nothing else to write to this fd... */

    conn_iterator = pool->conn_head;

    while(conn_iterator != NULL){

        if(conn_iterator->fd == sd){ /* send message to fd */

            /* this part needs improvement */
            /* printf("Processing msgs for connection : %d \n", conn_iterator->fd); */

            msg_iterator = conn_iterator->write_msg_head;

            while(msg_iterator != NULL){

                write_ret_val = write(conn_iterator->fd, msg_iterator->message, msg_iterator->size);

                if(write_ret_val == -1){
                    perror("Socket write error");
                    assert(0);
                }

//                if(write_ret_val == 0){
//                    fprintf(stderr, "Write to closed socket\n");
//                }

                free(msg_iterator->message);

                msg_iterator_to_delete = msg_iterator;

                msg_iterator = msg_iterator->next;

                free(msg_iterator_to_delete);


            } /* msg processing loop */

            conn_iterator->write_msg_head = NULL; /* Assume all messages are sent and clear msg buffer */
            FD_CLR(conn_iterator->fd, &pool->write_set); /* new loc for write fd_clear */

        }else{ /* connections related to other socket descriptors */

            /* this part needs improvement */
            /* printf("Skipping connection : %d \n", conn_iterator->fd); */

        }

        conn_iterator = conn_iterator->next;

    }

    return 0;
}
