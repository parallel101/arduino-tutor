#include "GlobalHTTP.h"
#include <HTTPClient.h>

HTTPClient *http;

void httpSetup() {
    http = new HTTPClient;
}
