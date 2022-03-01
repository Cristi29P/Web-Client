#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "parson.h"
#include "requests.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

char *basic_extract_json_response(char *str)
{
    return strstr(str, "{\"");
}

void register_user(int sockfd, char *server_ip, char *username, char *password) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // Trimitere efectiva
    char *message;
    char *response;

    message = compute_post_request(server_ip, "/api/v1/tema/auth/register", "application/json", &serialized_string, 1, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    
    char *flag = strstr(response, "Created");
    // Could not be created
    if (flag == NULL) {
        printf("An error occured. Register impossible right now.\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(message);
        free(response);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
        return;
    }

    printf("Register successful!\n");

    free(message);
    free(response);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

// Returns a cookie or a message error and NULL. Cookie memory must be freed inside the caller
char* login_user(int sockfd, char *server_ip, char *username, char *password) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    // Trimitere efectiva
    char *message;
    char *response;

    message = compute_post_request(server_ip, "/api/v1/tema/auth/login", "application/json", &serialized_string, 1, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");

    if (flag == NULL) {
        printf("An error occured. Login failed\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }
        free(message);
        free(response);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
        return NULL;
    }
  
    char *cookie_pos = strstr(response, "connect");
    char *actual_cookie = strtok(cookie_pos, ";");
    char *returned_cookie = (char *)calloc(strlen(actual_cookie) + 1, sizeof(char));
    DIE(returned_cookie == NULL, "Memory allocation failed\n");

    strcpy(returned_cookie, actual_cookie);

    free(message);
    free(response);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    printf("Login successful!\n");

    return returned_cookie;
}

// Returns a token or a message error and NULL. Token memory must be freed inside the caller
char* request_access(int sockfd, char *server_ip, char *cookie) {
    char *message;
    char *response;

    message = compute_get_request(server_ip, "/api/v1/tema/library/access", NULL, &cookie, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");

    if (flag == NULL) {
        printf("An error occured. Entering the library failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(message);
        free(response);
        return NULL;
    }

    char *jwt = basic_extract_json_response(response);

    JSON_Value *valoare= json_parse_string(jwt);
    JSON_Object* json_response = json_value_get_object(valoare);

    const char *token_aux = json_object_dotget_string(json_response, "token");
    char *token = (char *)calloc(strlen(token_aux) + 1, sizeof(char));
    DIE(token == NULL, "Token allocation failed!\n");
    strcpy(token, token_aux);

    free(message);
    free(response);
    json_value_free(valoare);
    printf("You have entered the library successfully!\n");
    return token;
}

void get_books(int sockfd, char *server_ip, char *token) {
    char *message;
    char *response;

    message = compute_get_request_token(server_ip, "/api/v1/tema/library/books", NULL, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");
    if (flag == NULL) {
        printf("An error occured. Retrieving books info failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(response);
        free(message);
        return;
    }

    char *info = basic_extract_json_response(response);
    if (info == NULL) {
        // Nu exista nimic in lista de json
        puts("There are no books!");
    } else {
        puts("These are the books: ");
        puts(info);
    }

    free(message);
    free(response);
}

void get_book(int sockfd, char *server_ip, char *token, int id) {
    char *message;
    char *response;

    char address[50];
    sprintf(address, "/api/v1/tema/library/books/%d", id);

    message = compute_get_request_token(server_ip, address, NULL, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");
    if (flag == NULL) {
        printf("An error occured. Retrieving book info failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(response);
        free(message);
        return;
    }

    char *info = basic_extract_json_response(response);
    if (info == NULL) {
        // Nu exista nimic in lista de json
        puts("There are no books!");
    } else {
        puts("This is your book: ");
        puts(info);
    }

    free(message);
    free(response);
}

void add_book(int sockfd, char *server_ip, char *token) {
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    // Datele ce trebuie citite
    char *title, *author, *genre, *publisher;
    size_t bufsize = 100;
    int page_count;
    title = (char *)calloc(LINEBUFF, sizeof(char));
    DIE(title == NULL, "Memory allocation failed.\n");
    author = (char *)calloc(LINEBUFF, sizeof(char));
    DIE(author == NULL, "Memory allocation failed.\n");
    genre = (char *)calloc(LINEBUFF, sizeof(char));
    DIE(genre == NULL, "Memory allocation failed.\n");
    publisher = (char *)calloc(LINEBUFF, sizeof(char));
    DIE(publisher == NULL, "Memory allocation failed.\n");

    // Citire date la tastatura
    printf("title=");
    getline(&title, &bufsize, stdin);
    printf("author=");
    getline(&author, &bufsize, stdin);
    printf("genre=");
    getline(&genre, &bufsize, stdin);
    printf("publisher=");
    getline(&publisher, &bufsize, stdin);
    printf("page_count=");
    int ret = scanf("%d", &page_count);

    if (ret != 1 || page_count < 0 || strlen(title) < 1 || strlen(author) < 1
    || strlen(genre) < 1 || strlen(publisher) < 1) {
        puts("Incomplete information. Abborting. Try again.");
        free(author);
        free(genre);
        free(publisher);
        free(title);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
        return;
    }

    title[strlen(title) - 1] = '\0';
    author[strlen(author) - 1] = '\0';
    genre[strlen(genre) - 1] = '\0';
    publisher[strlen(publisher) - 1] = '\0';

    // Creare payload tip JSON
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", page_count);
    json_object_set_string(root_object, "publisher", publisher);
    serialized_string = json_serialize_to_string_pretty(root_value);

    // Trimitere catre server
    char *message;
    char *response;

    message = compute_post_request_token(server_ip, "/api/v1/tema/library/books", "application/json", &serialized_string, 1, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");
    if (flag == NULL) {
        printf("An error occured. Adding a book failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }
        free(author);
        free(genre);
        free(publisher);
        free(title);
        json_free_serialized_string(serialized_string);
        json_value_free(root_value);
        free(response);
        free(message);
        return;
    }

    printf("Book added successfully!\n");

    free(message);
    free(response);
    free(author);
    free(genre);
    free(publisher);
    free(title);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

void delete_book(int sockfd, char *server_ip, char *token, int id) {
    char *message;
    char *response;

    char address[50];
    sprintf(address, "/api/v1/tema/library/books/%d", id);

    message = compute_delete_request_token(server_ip, address, NULL, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");
    if (flag == NULL) {
        printf("An error occured. Book deletion failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(response);
        free(message);
        return;
    }
    puts("Book deleted successfully!");

    free(message);
    free(response);
}

int logout_user(int sockfd, char *server_ip, char *cookie) {
    char *message;
    char *response;

    message = compute_get_request(server_ip, "/api/v1/tema/auth/logout", NULL, &cookie, 1);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    char *flag = strstr(response, "OK");
    if (flag == NULL) {
        printf("An error occured. Logout failed!\n");

        char *aux = basic_extract_json_response(response);

        if (aux != NULL) {
            printf("Here is the server response:\n %s\n", aux);
        } else {
            printf("Here is the server response:\n %s\n", response);
        }

        free(response);
        free(message);
        return -1;
    }


    puts("Logout successfully!");
    free(message);
    free(response);
    return 1;
}
