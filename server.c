/**************
* The server file that does the mail work 
* contains the http request 
*/ 

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
#include "server.h"

// Buffer used to manage the background size
static int buffer_backward_size = 0;
// no of players
static int no_of_players = 0;
// manage the players and its values
char player[2][10000][100] = {'\0'};
static int player_counter[2] = {0, 0};
bool match = false;
char browser_name[20][40] = {'\0'};
int browser_id = 0;
bool game_end = false;

// checks if game ends
bool game_finish(){
    for(int i = 0; i < player_counter[0]; i++){
        for(int j = 0; j < player_counter[1]; j++){
            if(!strcmp(player[0][i], player[1][j]))
                return true;
        }
    }
    return false;
}

// clears cookie buffer
void clearCookieBuffer(){
    for (int i = 0; i < 20; i++){
        bzero(browser_name[i], 40);
    }
}


// creates keywordbuffers
void createKeywordBuffer(char *buffer, int playerID){
    bzero(buffer, strlen(buffer));

    int n = sprintf(buffer, "%s", player[playerID][0]);

    for(int i = 1; i<player_counter[playerID]; i++){
        n = n + sprintf(&buffer[n], ", %s", player[playerID][i]);
    }
}

// check bowser cookies
bool check_browser_cookie (char *buff) {
    if(strstr(buff, "Cookie: browser=") != NULL) {
        return true;
    } else {
        return false;
    }  
}

// set browser cookies in ana array
int set_browser_cookie (char *buff, char *words) {
    int counter = 0;
    
    for(counter = 0; counter < 20; counter++){
        if(strlen(browser_name[counter]) == 0){
            strcpy(browser_name[counter], words);
            return counter;
        }
    }
    return counter;
}

// to find the browser id
int find_browser_id (char *buff) { 
    
    for(int i = 0; i < strlen(buff); i++) {
        if(!strncmp(&buff[i], "Cookie: browser=", 16)) {
            char * browser_word = NULL;
            browser_word = strstr(buff, "Cookie: browser=") + 16;
            char val[100]={'\0'};
            strcpy(val, browser_word);
            int id = atoi(val);
            return id;
        }
    }
    return 0;
}

void set_start_browser (char *buff, char *filename) {
    if(strstr(buff, "Cookie: browser=")) {
        strcpy(filename, START); 
        buffer_backward_size = 214;
    }   
}

void set_browser_name (int id, char *player_name) {
    strcpy(browser_name[id], player_name);
}

// get the word based on the index also
 char *get_browser_name (int id) {
    char *name = malloc(sizeof(char *));
    strcpy(name, browser_name[id]);

    return name;
 }

// find the cookie id
int findCookieId(char *buffer){
    char *c = strstr(buffer, "Cookie: browser=") + 16;
    return atoi(c);
}

// detect the icon
bool not_detect_icon (int sockfd, char *buff, int n) {
    if (strstr(buff, "favicon.ico")) {
        n = sprintf(buff, HTTP_404);
        if (write(sockfd, buff, n) < 0)
        {
            perror("write");
            return false;
        }
        return false;
    }
    return true;
}


void compare_latest (int thisPlayer, char *value) {
    int otherPlayer = 0;
    if(thisPlayer == 0) {
        otherPlayer = 1;
    } else {
         otherPlayer = 0;
    }
    int counter = player_counter[otherPlayer];
    for(int i = 1; i < counter; i++) {
       if(!strcmp(value, player[otherPlayer][i])) {
            match = true;   
        } 
    }
}

// update array
void update_array(int playerNum, char *value, int wordlength) {
    int counter = player_counter[playerNum];
    bzero(player[playerNum][counter], 100);
    strncpy(player[playerNum][counter], value, wordlength);
    player_counter[playerNum] += 1;
}

void reset_array() {
    for (int i = 0; i < 2; i++) {
        for(int j = 0; j < 10000; j++) {
            bzero(player[i][j], 100);
        }
    }  
}

bool check_user(char *buff) {
    if (strstr(buff, "user=")) {
        return true;    
    } else {
        return false;
    }
}

// measures the content and prepare content for HTML
void findType(char *buff, char *filename, char *words)
{
    char *c = buff;
    //char words[100] = {'\0'};
    char *temp = NULL;
    for (int i = 0; i < strlen(buff); i++)
    {
        if (!strncmp(c, "user=", 5))
        {
            temp = strstr(buff, "user=") + 5;
            strncpy(words, temp, strlen(temp));
            strcpy(filename, START); 
            buffer_backward_size = 214;
        }
        else if (!strncmp(c, "keyword=", 8))
        {
                if (no_of_players < 2)
                {
                    strcpy(filename, DISCARDED);
                    buffer_backward_size = 0;
                }
                else if (no_of_players == 2)
                {
                    strcpy(filename, ACCEPTED);
                    temp = strstr(buff, "keyword=") + 8;
                    strncpy(words, temp, strlen(temp)-12);
                    buffer_backward_size = 266;
                } else {
                    perror("more than 2 players\n");
                }
            
        }
        else if (!strncmp(c, "quit=Quit", 9))
        {
            temp = strstr(buff, "quit=Quit") + 9;
            strncpy(words, temp, strlen(temp));
            buffer_backward_size = 0;
            strcpy(filename, QUIT);
            no_of_players--; 
        }
        else
        {

        }
        c++;
    }
    //return &words[0];
}

// check if bowser is going to start
bool check_welcome_request(char *buff) {
    if (strstr(buff, "GET / HTTP/1.1") != NULL) {
        return true;
    }
    else {
        return false;
    }
}
// get files for the selector
void get_file_selector(char *buff, char *filename)
{   
    if (strstr(buff, "GET / HTTP/1.1"))
    {
        strcpy(filename, WELCOME);
    }
    else if (strstr(buff, "GET /?start=Start HTTP/1.1"))
    {
        if(no_of_players < 0) {
            no_of_players = 0;
        }
        strcpy(filename, FIRST_TURN);
        match = false;
        no_of_players++;
    }
}

// set the cookie buffer
bool setCooketBuffer(int sockfd, char *buff, char *name, int size){
    bzero(buff, 2048);
    int filefd = open(START, O_RDONLY);
    int n = read(filefd, buff, 2048);
    if (n < 0)
    {
        perror("read");
        close(filefd);
        return false;
    }
    close(filefd);

    int p1, p2;
    int added_length = strlen(name) + 4;
    size += added_length;
    for (p1 = size - 1, p2 = p1 - added_length; p1 >= size - buffer_backward_size; --p1, --p2) 
        buff[p1] = buff[p2];
    ++p2;

    if (buffer_backward_size != 0)
    {
        // put the separator
        buff[p2++] = '<';
        buff[p2++] = 'b';
        buff[p2++] = 'r';
        buff[p2++] = '>';
        // copy the username
        
        strncpy(buff + p2, name, strlen(name));
    }
    else
    {
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        char empty_array[999] = {'\0'};
        //reset_array(playerNum);
        strncpy(buff + p2, empty_array, strlen(name));
    }
    
    if (write(sockfd, buff, size) < 0)
    {
        perror("write");
        return false;
    }

    return true;

}

// handle get request
bool handle_get_request(int sockfd, char *buff, int n, int playerNum)
{
    char filename[50] = {'\0'};
    char buffer[2048] = {'\0'};
    strcpy(buffer, buff);
    if (check_browser_cookie(buff) && check_welcome_request(buff)) {

        browser_id = find_browser_id(buff);

        set_start_browser (buff, filename);
        struct stat st;
        stat(filename, &st);
        char *value = get_browser_name (browser_id);
        
        n = sprintf(buff, HTTP_200_FORMAT, st.st_size);

        if (write(sockfd, buff, n) < 0)
        {
            perror("write");
            return false;
        }
        // send the file
        if(check_browser_cookie(buffer)) {
            return setCooketBuffer(sockfd, buff, value, st.st_size);
        }
        int filefd = open(filename, O_RDONLY);
        do
        {
            n = sendfile(sockfd, filefd, NULL, 2048);
        } while (n > 0);
        if (n < 0)
        {
            perror("sendfile");
            close(filefd);
            return false;
        }
        close(filefd);

    } else {
        get_file_selector(buff, filename);
        // get the size of the file
        struct stat st;
        stat(filename, &st);
        n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
        if (write(sockfd, buff, n) < 0)
        {
            perror("write");
            return false;
        }
        // send the file
        int filefd = open(filename, O_RDONLY);
        do
        {
            n = sendfile(sockfd, filefd, NULL, 2048);
        } while (n > 0);
        if (n < 0)
        {
            perror("sendfile");
            close(filefd);
            return false;
        }
        close(filefd);
    }
    return true;
}

// handle post reuqest
bool handle_post_request(int sockfd, char *buff, int n)
{
    char filename[50] = {'\0'};
    // locate the value, it is safe to do so in this sample code, but usually the result is expected to be
    // copied to another buffer using strcpy or strncpy to ensure that it will not be overwritten.
    char words[100] = {'\0'};

    findType(buff, filename, words);
    char value[1000] = {'\0'};
    strcpy(value, words);
    if(strstr(buff, "keyword=")!= NULL){
        int id = findCookieId(buff);
        update_array(id, value, strlen(value));
        bzero(value, 1000);
        createKeywordBuffer(value, id);
        if(game_finish()) {
            no_of_players--;
            strcpy(filename, GAMEOVER);
            buffer_backward_size = 0;

            reset_array();    
        }
        
    }
    int word_length = strlen(value);
    // the length needs to include the ", " before the username
    long added_length = word_length + 4;
    // get the size of the file
    struct stat st;
    stat(filename, &st);
    // increase file size to accommodate the username
    long size = st.st_size + added_length;
    long cookie_size = strlen(HTTP_200_FORMAT_COOKIE);
    long total_size = size + cookie_size;
    if(!check_browser_cookie(buff) && check_user(buff)) {
        int id = set_browser_cookie(buff, value);
        set_browser_name (id, value);
        total_size = size + cookie_size;
        
        n = sprintf(buff, HTTP_200_FORMAT_COOKIE, total_size, (long)id);
    } else {
        total_size = size;
        n = sprintf(buff, HTTP_200_FORMAT, size);
    }
        // send the header first
    if (write(sockfd, buff, n) < 0)
    {
        perror("write");
        return false;
    }
    bzero(buff, 2048);
        // read the content of the HTML file
    int filefd = open(filename, O_RDONLY);
    n = read(filefd, buff, 2048);
    if (n < 0)
    {
        perror("read");
        close(filefd);
        return false;
    }
    close(filefd);
        // move the trailing part backward
    int p1, p2;
    for (p1 = size - 1, p2 = p1 - added_length; p1 >= size - buffer_backward_size; --p1, --p2) 
        buff[p1] = buff[p2];
    ++p2;

    if (buffer_backward_size != 0)
    {
        // put the separator
        buff[p2++] = '<';
        buff[p2++] = 'b';
        buff[p2++] = 'r';
        buff[p2++] = '>';
        // copy the vlue
        
        strncpy(buff + p2, value, word_length);
    }
    else
    {
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        buff[p2++] = ' ';
        char empty_array[999] = {'\0'};
        //reset_array(playerNum);
        strncpy(buff + p2, empty_array, word_length);
    }


        if (write(sockfd, buff, total_size) < 0)
        {
            perror("write");
            return false;
        }

    if(!strncmp(filename, QUIT, strlen(QUIT))) {
        return false;
    }
    return true;
}

bool handle_http_request(int sockfd, int playerNum)
{
    // try to read the request
    char buff[2048];
    bool request = true;
    int n = read(sockfd, buff, 2048);
    
    if (n <= 0)
    {
        if (n < 0)
            perror("read");
        else
            printf("socket %d close the connection\n", sockfd);
        return false;
    }

    // terminate the string
    buff[n] = 0;

    char *curr = buff;

    // parse the method
    METHOD method = UNKNOWN;
    if (strncmp(curr, "GET ", 4) == 0)
    {
        curr += 4;
        method = GET;
    }
    else if (strncmp(curr, "POST ", 5) == 0)
    {
        curr += 5;
        method = POST;
    }
    else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    // sanitise the URI
    while (*curr == '.' || *curr == '/')
        ++curr;
    // assume the only valid request URI is "/" but it can be modified to accept more files
    if (strlen(curr) > 0)
    {
        if (method == GET)
        {
            
            if(!not_detect_icon (sockfd, buff, n)) {
                
                return false;
            }

            handle_get_request(sockfd, buff, n, playerNum);
        }
        else if (method == POST)
        { 
            if(!not_detect_icon (sockfd, buff, n)) {
                
                return false;
            }
            request = handle_post_request(sockfd, buff, n);
        }
        else
            // never used, just for completeness
            fprintf(stderr, "no other methods supported");
    }
    // send 404
    else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    return request;
}
