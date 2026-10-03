// #include "core/https_socket.h"
// #include "core/cJSON.h"
#include <stdio.h>

int djb2_hash(const char* command);
int http_test(const char* url, const char* path);

int main(){
    //int h_t = http_test("api.live.prod.thehelldiversgame.com", "/api/v2/Assignment/War/801");
    int hash = djb2_hash("toss");
}

int djb2_hash(const char* command){
    int hash = 5381;
    const char* ptr = command;
    int c;
    while((c = *ptr++)){
        if(c == ' ')
            break;
        hash = ((hash << 5) + hash) + c;
    }
    printf("%s: %d\n", command, hash);
    return hash;
}

// int http_test(const char* url, const char* path){
//     https_ctx_init();
//     struct https_socket sock;
//     if(https_connect(&sock, url, "443")){
//         return 1;
//     }
//     //api.live.prod.thehelldiversgame.com/api/v2/Assignment/War/801
//     char request[512];
//     snprintf(request, 512,
//         "GET %s HTTP/1.1\r\n"
//         "Accept-Language: en-US\r\n"
//         "Host: %s\r\n"
//         "\r\n"
//     , path, sock.hostname);
//     char* response = https_send(&sock, request);
//     if(!response){
//         https_close(&sock);
//         return 1;
//     }
//     struct cJSON* json = cJSON_Parse(response);
//     char* res_json = cJSON_Print(json);
//     printf("%s", res_json);
//     free(res_json);
//     cJSON_Delete(json);
//     https_ctx_free();
//     return 0;
// }