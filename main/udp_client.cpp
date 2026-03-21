#include "udp_client.h"

#include <string.h>
#include <errno.h>

#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

static const char *TAG = "udp_client";
static const char *SERVER_IP = "192.168.4.1";
static const int SERVER_PORT = 3333;

void udp_send_command(const char *cmd)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno=%d", errno);
        return;
    }

    struct sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &dest.sin_addr);

    int err = sendto(sock, cmd, strlen(cmd), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (err < 0) {
        ESP_LOGE(TAG, "sendto() failed: errno=%d", errno);
    }

    close(sock);
}