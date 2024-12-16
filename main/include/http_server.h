#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
#include "esp_netif.h"
#include <esp_http_server.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

/**
 * @brief Initialize netif stack, create handlers for wifi events. Set parameters for the network connection.
 **/
esp_err_t http_server_init();

/**
 * @brief Connect to the wifi after initialization.
 **/
esp_err_t http_server_connect();