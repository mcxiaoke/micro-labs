#ifndef _DATA_H
#define _DATA_H

#include <Arduino.h>

static const char PROGMEM UPDATE_PAGE_TPL[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <title>OTA Update</title>
</head>

<body>
    <form method='POST' action='/update' enctype='multipart/form-data'>
        <input type='file' name='update'>
        <input type='submit' value='Update'>
    </form>
</body>

</html>
)rawliteral";

static const char PROGMEM ROOT_PAGE_TPL[] = R"rawliteral(
<html lang="en">
</html>
)rawliteral";

static const char PROGMEM LOGS_PAGE_TPL[] = R"rawliteral(
<html lang="en">
</html>
)rawliteral";

#endif