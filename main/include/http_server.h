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
 * @brief This function is responsible for setting up both wifi connection and set up a http server. 
 * It should be called as simultanous task.
 **/
void http_server_main(void);