// const HumanElapsed = require('human-elapsed')
const baseUrl = "http://192.168.100.7";

var parseHTML = function (str) {
    var tmp = document.implementation.createHTMLDocument();
    tmp.body.innerHTML = str;
    return tmp.body.children;
};

var humanElapsedMs = (sec) => humanElapsed(sec / 1000);

// in seconds
function humanElapsed(sec) {
    // 4010
    // 4010/3600 = 1h
    // 4010%3600 = 410
    // 410/60 = 6m
    // 410%60 = 50
    // 50s
    var h = Math.floor(sec / 3600);
    var m = Math.floor((sec % 3600) / 60);
    var c = Math.floor(sec % 60);
    var s = "";
    if (h > 0) {
        s += h;
        s += "h";
    }
    if (m > 0) {
        if (s.length > 1) {
            s += " ";
        }
        s += m;
        s += "m";
    }
    if (c > 0) {
        if (s.length > 1) {
            s += " ";
        }
        s += c;
        s += "s";
    }
    return s;
};

function buildOutputDiv(d) {
    var gp = $('<p>').attr('id', 'p-global')
        .text("Global Switch: " + (d["switch"] ? "On" : "Off")
            + (d["debug"] ? "(Debug)" : ""))

    var pp = $('<p>').attr('id', 'p-status')
        .text("Pump Status: " + (d["on"] ? "Running" : "Idle") + " (" +
            moment.unix(d["ts"]).format("YYYY-MM-DD HH:mm:ss") + ")")

    var lp = $('<p>').attr('id', 'p-last')
        .text("Last RunAt: " + moment.unix(d["lastAt"]).format("YYYY-MM-DD HH:mm:ss")
            + ", Elapsed: " + d["lastElapsed"] + "s")

    var np = $('<p>').attr('id', 'p-next')
        .text("Next RunAt: " +
            moment.unix(d["nextAt"]).format("YYYY-MM-DD HH:mm:ss") +
            ", Remains: " + humanElapsed((d["nextAt"] - moment().unix())))

    var tp = $('<p>').attr('id', 'p-task')
        .text("Task Interval: " + humanElapsed(d["interval"] / 1000)
            + ", Task Duration: " + humanElapsed(d["duration"] / 1000))

    var o = $('<div>').attr('id', 'output').attr('class', 'output');
    return o.append(gp, pp, lp, np, tp, $('<hr>'));
}

function buildFormDiv(d) {

    var pf = $('<form>')
        .attr('id', 'pump-form')
        .attr('method', 'POST')
        .attr('action', baseUrl + '/j/toggle_pump?action=' + (d["on"] ? "off" : "on"))
        .append(
            $('<button>').attr('id', 'pump_submit').attr('type', 'submit')
                .text(d["on"] ? "Stop Pump" : "Start Pump")
        )

    var sf = $('<form>')
        .attr('id', 'switch-form')
        .attr('method', 'POST')
        .attr('action', baseUrl + '/j/toggle_switch?action=' + (d["switch"] ? "off" : "on"))
        .append(
            $('<button>').attr('id', 'switch_submit').attr('type', 'submit')
                .text(d["switch"] ? "Switch Off" : "Switch On")
        )

    return $('<div>').attr('id', 'form-div').append(pf, sf, $('<p>'));
}

function buildButtonDiv(d) {
    var buttonDiv = document.createElement("div");
    buttonDiv.setAttribute("id", "button-div");

    var btnLogs = document.createElement("button");
    btnLogs.textContent = "View Logs";
    btnLogs.setAttribute("id", "btn-logs");
    btnLogs.onclick = e => (window.location.href = baseUrl + "/www/logs.html");

    var btnRaw = document.createElement("button");
    btnRaw.textContent = "Raw Logs";
    btnRaw.setAttribute("id", "btn-raw");
    btnRaw.onclick = e => (window.location.href = baseUrl + "/file/pump.log");

    var btnClear = document.createElement("button");
    btnClear.textContent = "Clear Logs";
    btnClear.setAttribute("id", "btn-clear");
    // btnClear.setAttribute("type", "submit");
    btnClear.onclick = function (e) {
        var cf = confirm("Are you sure to delete all logs?");
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function () {
                if (xhr.readyState == 4) {
                    if (xhr.status < 300) {
                        alert('Server logs is cleared!');
                        console.log("Clear logs ok.");
                    }
                }
            };
            xhr.open("POST", baseUrl + "/j/clear_logs");
            xhr.send();
            return true;
        }
        return false;
    };

    var btnCmd = document.createElement("button");
    btnCmd.textContent = "Commands";
    btnCmd.setAttribute("id", "btn-cmd");
    btnCmd.onclick = e => (window.location.href = baseUrl + "/www/cmd.html");
    buttonDiv.append(btnLogs, btnRaw, btnClear, btnCmd);
    return buttonDiv;
}

function loadData() {
    function handleError(e) {
        $('#content').append($('<p>').text('Failed to load data.'));
    }

    var xhr = new XMLHttpRequest();
    xhr.timeout = 3000;
    xhr.ontimeout = function (e) {
        console.log('ontimeout');
        handleError(e);
    }
    xhr.onerror = function (e) {
        console.log('onerror');
        handleError(e);
    }
    xhr.onloadend = function (e) {
        var output = document.createElement('div');
        output.innerHTML = document.getElementById('content').outerHTML;
        console.log(output);
    }
    xhr.onload = function (e) {
        console.log('onload ' + xhr.status);
        if (xhr.status < 400) {
            var d = JSON.parse(xhr.responseText);
            console.log(d);
            var c = document.getElementById("content");
            var t = document.createElement("h1");
            t.textContent = "Pump Home";
            // c.append(t, buildOutputDiv(d), buildFormDiv(d), buildButtonDiv(d));

            $('#content').append(t, buildOutputDiv(d), buildFormDiv(d), buildButtonDiv(d));
        } else {
            var c = document.getElementById("content");
            var t = document.createElement('p');
            t.textContent = "ERROR " + xhr.status;
            c.appendChild(t);
        }
    };
    xhr.open("GET", baseUrl + "/j/get_status_json");
    xhr.send();
    return false;
}

function onReady(e) {
    loadData();
}
window.addEventListener("DOMContentLoaded", onReady);
