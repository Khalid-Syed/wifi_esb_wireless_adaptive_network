#include "time_sync.h"

struct timespec ts;
struct tm tm_val;
TimePacket pck;
static int64_t utc_offset_sec = 0;  // Offset = actual_utc_time - k_uptime


void update_utc_offset(TimePacket pck) {
    struct tm tm_server = {
        .tm_year = pck.year - 1900,
        .tm_mon  = pck.month - 1,
        .tm_mday = pck.day,
        .tm_hour = pck.hour,
        .tm_min  = pck.minute,
        .tm_sec  = pck.second
    };

    time_t server_epoch = timeutil_timegm(&tm_server);  // Convert UTC to epoch seconds
    int64_t uptime_sec = k_uptime_get() / 1000;

    utc_offset_sec = server_epoch - uptime_sec;

}

int fetch_time_from_server(void) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[128];
    int received;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TIME_SERVER_PORT);
    inet_pton(AF_INET, TIME_SERVER_IP, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }

    received = recv(sock, &pck, sizeof(TimePacket) - 1, 0);
    if (received > 0) 
    {
        buffer[received] = '\0';
    } 
    else 
    {
        return -1;
    }

    update_utc_offset(pck);  // Call this ONCE after receiving UTC time

    struct tm new_tm_val = {
        .tm_year = pck.year - 1900,
        .tm_mon = pck.month - 1,
        .tm_mday = pck.day,
        .tm_hour = pck.hour,
        .tm_min = pck.minute,
        .tm_sec = pck.second,
    };

    time_t unix_time = timeutil_timegm(&new_tm_val);
    if (unix_time == (time_t)-1) 
    {
        return -1;
    } 
    else 
    {
        struct timespec new_ts = 
        {
            .tv_sec = unix_time,
            .tv_nsec = pck.usec * 1000,
        };

        if (clock_settime(CLOCK_REALTIME, &new_ts) < 0) 
        {
            return -1;
        }
    }

    close(sock);

    return 0;
}



TimePacket get_current_time(void) {
    int64_t uptime_ms = k_uptime_get();
    int64_t now_utc_sec = (uptime_ms / 1000) + utc_offset_sec;
    long usec = (uptime_ms % 1000) * 1000;


    ts.tv_sec = now_utc_sec;
    ts.tv_nsec = usec * 1000;

    gmtime_r(&ts.tv_sec, &tm_val);  // Convert to broken-down UTC time

    pck.year = (tm_val.tm_year + 1900);
    pck.month = (tm_val.tm_mon + 1);
    pck.day = (tm_val.tm_mday);
    pck.hour = (tm_val.tm_hour);
    pck.minute = (tm_val.tm_min);
    pck.second = (tm_val.tm_sec);
    pck.usec = usec;

    return pck;
}

char* get_current_time_string() {
    int64_t uptime_ms = k_uptime_get();
    int64_t now_utc_sec = (uptime_ms / 1000) + utc_offset_sec;
    long usec = (uptime_ms % 1000) * 1000;

    struct timespec ts;
    struct tm tm_val;
    ts.tv_sec = now_utc_sec;
    ts.tv_nsec = usec * 1000;

    gmtime_r(&ts.tv_sec, &tm_val);  // convert to UTC broken-down time

    char *buffer = malloc(32);  // ISO8601 string with ms fits in ~25 chars
    if (buffer == NULL) {
        return "";  // malloc failed
    }

    snprintf(buffer, 32, "%04d-%02d-%02dT%02d:%02d:%02d:%03ld",
             tm_val.tm_year + 1900,
             tm_val.tm_mon + 1,
             tm_val.tm_mday,
             tm_val.tm_hour,
             tm_val.tm_min,
             tm_val.tm_sec,
             usec / 1000); // convert microseconds to milliseconds
             
    return buffer;  // Caller must free
}