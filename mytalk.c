#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <pwd.h>
#include <poll.h>
#include <signal.h>
#include "mytalk.h"
#include "talk.h"

#define DECIMAL 10
#define MAX_Q 10
#define HOSTMAX 253
#define ACCEPTANCE_SIZE 3
#define RESPONSE_SZ 2
#define GREETING_SIZE 25
#define TRUE 1
#define FALSE 0
#define BUFMAX 1024

/* Linux/Unix define max hostname to be 253 chars */
/* hostname(7) */


int main(int argc, char* argv[]) {

    int sockfd, socknew, n;
    struct sockaddr_in addr;
    struct addrinfo hints, *res;

    if (argc < 2) {
        printf("usage: [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
        exit(1);
    }
    
    int opt;
    int args = 0;
    int verbose = 0;
    int no_window = 0;
    int no_request = 0;
    while ((opt = getopt(argc, argv, "vaN")) != -1) {
        switch (opt) {
            case 'v':
                args++;
                verbose = 1;
                break;
            case 'N':
                /* Cannot be repeated */
                args++;
                if (no_window == 1) {
                    printf("usage: [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
                    if (verbose == 1) {
                        printf("-N cannot be repeated\n");
                    }
                    exit(1);
                }
                no_window = 1;
                break;
            case 'a':
                /* Cannot be repeated */
                args++;
                if (no_request == 1) {
                    printf("usage: [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
                    if (verbose == 1) {
                        printf("-a cannot be repeated\n");
                    }
                    exit(1);
                }
                no_request = 1;
                break;
            default:
                printf("usage: [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
                exit(1);
        }
    }

    char* ptr = NULL;
    long portno;
    portno = strtol(argv[argc - 1], &ptr, DECIMAL);
    if (strlen(ptr) > 0) {
        if (verbose == 1) {
            printf("Missing port\n");
        }
        printf("usage: [ -v ] [ -a ] [-N ] [ hostname ] port\n");
        exit(1);
    }

    /* Client */

    if (argc == args + 3) {
        /* If hostname is present */
        if (verbose == 1) {
            printf("Acting as client...\n");
        }
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(argv[argc - 2], argv[argc - 1], &hints, &res) != 0) {
            perror("getaddrinfo");
            exit(1);
        }
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(1);
        }
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
            perror("connect");
            exit(1);
        }
    
        printf("Waiting for response from %s\n", argv[argc - 2]);

        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);

        char* userhost = (char*) malloc(strlen(pw->pw_name) + strlen(argv[argc - 2]) + 2);
        strcpy(userhost, pw->pw_name);
        strcat(userhost, "@");
        strcat(userhost, argv[argc - 2]);
        userhost[strlen(userhost)] = '\0';

        if (send(sockfd, userhost, strlen(userhost), 0) < 0) {
            perror("send");
            exit(1);
        }

        char* buffer = malloc(sizeof(char) * RESPONSE_SZ + 1);
        memset(buffer, 0, RESPONSE_SZ + 1);
        if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
            perror("recv");
            exit(1);
        }
        
        buffer[RESPONSE_SZ] = '\0';
        
        if (strcmp(buffer, "ok") == 0) {
            if (verbose == 1) {
                printf("Connection accepted\n");
            }
            printf("%s\n", buffer);
            free(buffer);
            communicate(sockfd, no_window);
        } 
        else {
            printf("%s declined connection.\n", argv[argc - 2]);
            close(sockfd);
            free(buffer);
            exit(1);
        }
    }

    /* Server */

    else if (argc == args + 2) {
        if (verbose == 1) {
            printf("Acting as server...\n");
        }
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Socket");
            exit(1);
        }
        else {
            if (verbose == 1)
                printf("Socket success...\n");
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(portno);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
            perror("Bind");
            exit(1);
        }
        else {
            if (verbose == 1)
                printf("Bind success...\n");
        }
        if (listen(sockfd, MAX_Q) < 0) {
            perror("Listen");
            exit(1);
        }
        else {
            if (verbose == 1)
                printf("Listening...\n");
        }
        if (verbose == 1) {
            printf("Waiting for connection...\n");
        }

        socklen_t addrlen = sizeof(addr);
        socknew = accept(sockfd, (struct sockaddr*) &addr, &addrlen);
        if (socknew < 0) {
            perror("Accept");
            exit(1);
        }
        else {
            if (verbose == 1)
                printf("Connection accepted!\n");
        }

        char* username = malloc(HOSTMAX + 1);
        if (recv(socknew, username, HOSTMAX + 1, 0) < 0) {
            perror("recv");
            exit(1);
        }

        if (no_request == 0) {
            printf("Mytalk request from %s. Accept (y/n)?  ", username);
            char s[3];
            int i;
            scanf("%s", s);
            while (strlen(s) > ACCEPTANCE_SIZE) {
                printf("Invalid response\n");
                printf("Accept (y/n)? ");
                scanf("%s", s);
            }
            for (i = 0; i < strlen(s); i++) {
                s[i] = tolower(s[i]);
            }

            if (strcmp(s, "yes") == 0 || strcmp(s, "y") == 0) {
                if (verbose == 1) {
                    printf("Connection accepted\n");
                }

                char* response = "ok";
                if (send(socknew, response, strlen(response), 0) < 0) {
                    perror("Send");
                    exit(1);
                }
                else {
                    if (verbose == 1)
                        printf("Sent response!\n");
                }

                /* Communicate */

                communicate(socknew, no_window);
            }
            else {
                if (verbose == 1) {
                    printf("Connection declined\n");
                }
                char* response = "no";
                if (send(socknew, response, strlen(response), 0) < 0) {
                    perror("Send");
                    exit(1);
                }
                else {
                    if (verbose == 1)
                        printf("Sent response!\n");
                }
            }
        }
        else {
            if (verbose == 1) {
                printf("Connecting without request...\n");
            }
            char* response = "ok";
            if (send(socknew, response, strlen(response), 0) < 0) {
                perror("Send");
                exit(1);
            }
            else {
                if (verbose == 1)
                    printf("Sent response!\n");
            }
            communicate(socknew, no_window);
        }

        close(sockfd);
    }
    else {
        printf("usage: [ -v ] [ -a ] [ -N ] [ hostname ] port\n");
        exit(1);
    }

    return 0;
}


void communicate(int sockfd, int no_window) {
    long i = 0;
    struct pollfd polls[2];
    polls[0].fd = STDIN_FILENO;
    polls[0].events = POLLIN;
    polls[0].revents = 0;
    polls[1].fd = sockfd;
    polls[1].events = POLLIN;
    polls[1].revents = 0;

    if (no_window == 0) {
        start_windowing();
    }

    int done;
    int len, mlen;
    char* buffer = malloc(sizeof(char) * BUFMAX + 1);
    done = 0;
    do {
	    poll(polls, 2, -1);
        update_input_buffer();
	    if (polls[0].revents & POLLIN) {
            /* Line has been typed in stdin - send line*/
            if (has_whole_line() == TRUE) {
                /* Wait for whole line */
                len = read_from_input(buffer, BUFMAX);
                buffer[len] = '\0';
                if (len < 0) {
                    perror("read");
                }
                else if (len == 0) {
                    done = 1;
                }
                else {
                    if (send(sockfd, buffer, len + 1, 0) < 0) {
                        perror("send");
                        exit(1);
                    }
                }
            }
	    }
	    if (polls[1].revents & POLLIN) {
	        /* Line needs to be received and printed to stdout */
            if ((mlen = recv(sockfd, buffer, len, 0)) < 0) {
                perror("recv");
 	        }
            
            fprint_to_output("%s", buffer);
            memset(buffer, 0, BUFMAX + 1);
        }
	    if (has_hit_eof() == TRUE) {
	        /* Figure out how to end convo */
            done = 1;
            char* terminate = "Connection closed. ^C to terminate.";
            if (send(sockfd, terminate, strlen(terminate), 0) < 0) {
                perror("send");
                exit(1);
            }
            close(sockfd);
            free(buffer);
        }
        
    } while (done == 0);

    stop_windowing();

}

