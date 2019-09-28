#ifndef _DATA_H
#define _DATA_H

#include <Arduino.h>

static const char PROGMEM ROOT_PAGE_TPL[] =  R"rawliteral(
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="X-UA-Compatible" content="ie=edge">
    <title>Water Pump Server</title>
    <style>
        .content {
            max-width: 640px;
            margin: auto;
        }
    </style>
</head>

<body>
    <div class="content">
        <h1>Water Pump Server</h1>
        <p>{{DATE_TIME}}</p>
        <p>LED Status: {{LED_LABEL}}</p>
        <p>Pump Status: {{PUMP_LABEL}}</p>
        <p>Last RunAt: {{PUMP_LAST_RUN_AT}}</p>
        <p>Last Duration: {{PUMP_LAST_RUN_DURATION}}</p>
        <form action="/led?action={{LED_ACTION}}" method="POST">
            <input type="submit" value="{{LED_SUBMIT}}">
        </form>
        <form action="/pump?action={{PUMP_ACTION}}" method="POST">
            <input type="submit" value="{{PUMP_SUBMIT}}">
        </form>
        <form action="/reset" method="POST">
            <input type="submit" value="Reboot">
        </form>
        <form action="/clear" method="POST">
            <input type="submit" value="Clear Logs">
        </form>
        <p><a href="/logs"><button>View Logs</button></a></p>
        <p><a href="/"><button>Refresh Page</button></a></p>
    </div>
</body>

</html>
)rawliteral";




static const char PROGMEM LOGS_PAGE_TPL[] =  R"rawliteral(
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="X-UA-Compatible" content="ie=edge">
    <title>Pump Server Logs</title>
    <style>
        .content {
            max-width: 640px;
            margin: auto;
        }

        table,
        th,
        td {
            border: 1px dashed lightgray;
            font-size: 90%;
        }
    </style>
</head>

<body>
    <div class="content">
        <h4>Pump Logs</h4>
        <table>
            {{PUMP_LOGS}}
        </table>
        <p><a href="/"><button>Back to Home</button></a></p>
        <p><a href="/"><button>Refresh Page</button></a></p>
    </div>
</body>

</html>
)rawliteral";

#endif