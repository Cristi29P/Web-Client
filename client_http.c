#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define HOST "34.118.48.238"
#define PORT 8080

int main() {
    char command[50];
    int sockfd = -1;
    char *cookie = NULL;
    char *token = NULL;

    while(1) {
        scanf("%s", command);
        int set = setvbuf(stdin , NULL , _IONBF , 0);
        DIE(set != 0, "Setvbuf failed!\n");
        

        if (strcmp(command, "register") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            // A session is currently active/ cookie present. Another register is impossible
            if (cookie != NULL) {
                printf("Error registering! You are already logged in!\n");
                close(sockfd);
                continue;
            }

            char *username = (char *)calloc(LINEBUFF, sizeof(char));
            DIE(username == NULL, "Memory allocation failed.\n");
            char *password = (char *)calloc(LINEBUFF, sizeof(char));
            DIE(password == NULL, "Memory allocation failed.\n");
            size_t bufsize = 100;
            // Getting the credentials
            printf("username=");
            fgets(username, bufsize, stdin);
            printf("password=");    
            fgets(password, bufsize, stdin);
            
            // Checking the constraints
            if (strlen(username) == 1 || strlen(password) == 1) {
                printf("Username/password empty! Try again!\n");
                free(username);
                free(password);
                close(sockfd);
                continue;
            }
            
            // Removing trailing \n
            username[strlen(username) - 1] = '\0';
            password[strlen(password) - 1] = '\0';

            register_user(sockfd, HOST, username, password);
            
            close(sockfd);
            free(username);
            free(password);
        }

        else if (strcmp(command, "login") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            // A session is currently active/ cookie present
            if (cookie != NULL) {
                printf("You are already logged in!\n");
                close(sockfd);
                continue;
            }

            char *username = (char *)calloc(LINEBUFF, sizeof(char));
            DIE(username == NULL, "Memory allocation failed.\n");
            char *password = (char *)calloc(LINEBUFF, sizeof(char));
            DIE(password == NULL, "Memory allocation failed.\n");
            size_t bufsize = 100;
            // Getting the credentials
            printf("username=");
            fgets(username, bufsize, stdin);
            printf("password=");    
            fgets(password, bufsize, stdin);

            // Checking the constraints
            if (strlen(username) == 1 || strlen(password) == 1) {
                printf("Username/password empty!\n");
                free(username);
                free(password);
                close(sockfd);
                continue;
            } 

            // Removing trailing \n
            username[strlen(username) - 1] = '\0';
            password[strlen(password) - 1] = '\0';

            cookie = login_user(sockfd, HOST, username, password);
            // Checking if the function returns a cookie on success
            if (cookie == NULL) {
                printf("Cookie NULL. Please try again to enter your credentials.\n");
                close(sockfd);
                free(username);
                free(password);
                continue;
            }

            close(sockfd);
            free(username);
            free(password);
        }

        else if (strcmp(command, "enter_library") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            // A session is currently active/ token present
            if (token != NULL) {
                printf("You have already entered the library!\n");
                close(sockfd);
                continue;
            }

            token = request_access(sockfd, HOST, cookie);
            // Checking if the function returns a token on success
            if (token == NULL) {
                printf("Token NULL. Make sure the cookie was correct.\n");
                close(sockfd);
                continue;
            }

            close(sockfd);
        }

        else if (strcmp(command, "get_books") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            get_books(sockfd, HOST, token);

            close(sockfd);
        }

        else if (strcmp(command, "get_book") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            int id;
            printf("Enter the book id. It must be a number greater than zero!\n");
            printf("id=");
            int ret = scanf("%d", &id);
            // Checking the constraints
            if (ret != 1 || id < 0) {
                puts("Invalid ID or scanf error. Please try again.");
                close(sockfd);
                continue;
            }

            get_book(sockfd, HOST, token, id);

            close(sockfd);
        }

        else if (strcmp(command, "add_book") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            add_book(sockfd, HOST, token);

            close(sockfd);
        }

        else if (strcmp(command, "delete_book") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            int id;
            printf("Enter the book id. It must be a number greater than zero!\n");
            printf("id=");
            int ret = scanf("%d", &id);
            // Checking the constraints
            if (ret != 1 || id < 0) {
                puts("Invalid ID or scanf error. Please try again.");
                close(sockfd);
                continue;
            }

            delete_book(sockfd, HOST, token, id);

            close(sockfd);
        }

        else if (strcmp(command, "logout") == 0) {
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            DIE(sockfd < 0 , "Connection could not be established!\n");

            int ret = logout_user(sockfd, HOST, cookie);

            // Checking if the logout was successful
            if (ret == -1) {
                close(sockfd);
                continue;
            }

            free(cookie);
            free(token);
            cookie = NULL;
            token = NULL;
            close(sockfd);   
        }

        else if (strcmp(command, "exit") == 0) {
            if (token != NULL) {
                free(token);
            }
            if (cookie != NULL) {
                free(cookie);
            }
            break;
        }

        else {
            puts("Wrong command!");
        }
    }

    return 0;
}
