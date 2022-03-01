#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000
#define LINEBUFF 100

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)


// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

void register_user(int sockfd, char *server_ip, char *username, char *password);

char* login_user(int sockfd, char *server_ip, char *username, char *password);

char* request_access(int sockfd, char *server_ip, char *cookie);

void get_books(int sockfd, char *server_ip, char *token);

void get_book(int sockfd, char *server_ip, char *token, int id);

void add_book(int sockfd, char *server_ip, char *token);

void delete_book(int sockfd, char *server_ip, char *token, int id);

int logout_user(int sockfd, char *server_ip, char *cookie);

#endif
