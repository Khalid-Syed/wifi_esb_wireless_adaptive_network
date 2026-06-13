
#ifndef HTTP_UPDATE__
#define HTTP_UPDATE__

#define TLS_SEC_TAG 42

#ifndef CONFIG_USE_HTTPS
#define SEC_TAG (-1)
#else
#define SEC_TAG (TLS_SEC_TAG)
#endif

#define HTTP_UPDATE_SESSION_ID_FILENAME "dfu_session_id" // session id of last update to prevent dupe downloads
/** Begin downloading specified FOTA update
 */
void http_update_start(int session_id, char *host, char *filename, char *install, int retries);

/** Notify the library that the update has been stopped.
 *
 */
void http_update_stop(void);

/** Notify the library that the update has been completed.
 *
 */
void http_update_done(void);

/**
 * @brief called when first connected to mqtt broker, often after a reboot
 *
 */

void http_update_connected(void);

/**
 * @brief send http update status
 *
 */
void http_update_status_send(void);

#endif /* HTTP_UPDATE__ */
