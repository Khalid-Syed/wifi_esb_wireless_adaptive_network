
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include "app_version.h"

// TODO: better semantic versioning from build process and git checkin sha

char *app_version_get(void)
{
    return CONFIG_APP_VERSION;
}
