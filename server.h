#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// constants
static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_200_FORMAT_COOKIE = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\
Set-Cookie: browser=%ld\r\n\r\n";
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;

static char const * const WELCOME = "1_intro.html";
static char const * const START = "2_start.html";
static char const * const FIRST_TURN = "3_first_turn.html";
static char const * const ACCEPTED = "4_accepted.html";
static char const * const DISCARDED = "5_discarded.html";
static char const * const GAMEOVER = "6_endgame.html";
static char const * const QUIT = "7_gameover.html";
// represents the types of method
extern char browser_name[20][40];
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;

// some universal methods
bool handle_http_request(int sockfd, int playerNum);
void clearCookieBuffer();
void reset_array();