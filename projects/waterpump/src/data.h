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

        form {
            display: inline-block;
        }
    </style>
</head>

<body>
    <div class="content">
        <h1>Water Pump</h1>
        <hr>
        <p>{{DATE_TIME}}</p>
        <p>LED Status: {{LED_LABEL}}</p>
        <p>Pump Status: {{PUMP_LABEL}}</p>
        <p>Last RunAt: {{PUMP_LAST_RUN_AT}}</p>
        <p>Last Duration: {{PUMP_LAST_RUN_DURATION}}</p>
        <hr>
        <form action="/led?action={{LED_ACTION}}" method="POST">
            <input type="submit" value="{{LED_SUBMIT}}">
        </form>
        <form action="/pump?action={{PUMP_ACTION}}" method="POST">
            <input type="submit" value="{{PUMP_SUBMIT}}">
        </form>
        <a href="/logs"><button>Logs</button></a>
        <a href="/"><button>Refresh</button></a>
        <form action="/reset" method="POST">
            <input type="submit" value="Reboot" onclick="return confirm('Are you sure to do chip reset ?')">
        </form>
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
    <title>Pump Logs</title>
    <style>
        .content {
            max-width: 640px;
            margin: auto;
        }

        form {
            display: inline-block;
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
        <div>
            <a href="/"><button>Home</button></a>
            <a href="/logs"><button>Refresh</button></a>
            <form action="/clear" method="POST">
                <input type="submit" value="Clear" onclick="return confirm('Are you sure to delete all logs ?')">
            </form>
        </div>
        <hr>
        <table>
            {{PUMP_LOGS}}
        </table>
        <hr>
    </div>
</body>

</html>
)rawliteral";

#endif