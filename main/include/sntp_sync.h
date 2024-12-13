 #include "esp_err.h"

/**
 * @brief connect with the SNTP server to synchronize time.
 * @return ESP_OK if connection to sntp server is established.
 */
 esp_err_t sntp_sync_init();


 /**
 * @brief Time actualization.
 * @param bufTime buffer into which will be saved data and time, in format dd/mm/yyyy hh:mm:ss
 * @return ESP_OK if data has been obtained from the sntp server.
 */
 esp_err_t sntp_sync_ObtainActualTime(const char* bufTime);