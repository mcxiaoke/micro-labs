const baseUrl = "http://192.168.100.4";

var parseHTML = function (str) {
    var tmp = document.implementation.createHTMLDocument();
    tmp.body.innerHTML = str;
    return tmp.body.children;
};

function getUrlParameter(name) {
    name = name.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
    var regex = new RegExp('[\\?&]' + name + '=([^&#]*)');
    var results = regex.exec(location.search);
    return results === null ? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
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

    var mp = $('<p>').attr('id', 'p-mem')
        .text('Free Stack: ' + d['stack'] + ', Free Heap: ' + d['heap']);

    var o = $('<div>').attr('id', 'output').attr('class', 'output');
    return o.append(gp, pp, lp, np, tp, mp, $('<hr>'));
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

function buildButtonDiv() {
    var buttonDiv = document.createElement("div");
    buttonDiv.setAttribute("id", "button-div");

    var btnLogs = document.createElement("button");
    btnLogs.textContent = "View Logs";
    btnLogs.setAttribute("id", "btn-logs");
    btnLogs.onclick = e => (window.location.href = baseUrl + "/www/logs.html");

    var btnRaw = document.createElement("button");
    btnRaw.textContent = "Raw Logs";
    btnRaw.setAttribute("id", "btn-raw");
    btnRaw.onclick = e => {
        window.open('/file/pump.log', '_blank')
    };

    var btnClear = document.createElement("button");
    btnClear.textContent = "Clear Logs";
    btnClear.setAttribute("id", "btn-clear");
    // btnClear.setAttribute("type", "submit");
    btnClear.onclick = function (e) {
        var cf = confirm("Are you sure to delete all logs?");
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onload = function () {
                if (xhr.status < 400) {
                    alert('Server logs is cleared!');
                    console.log("Clear logs ok.");
                }
            };
            xhr.open("POST", baseUrl + "/j/clear_logs");
            xhr.send();
            return true;
        }
        return false;
    };

    var btnFiles = document.createElement("button");
    btnFiles.textContent = "List Files";
    btnFiles.setAttribute("id", "btn-files");
    btnFiles.onclick = e => (window.location.href = baseUrl + "/www/files.html");

    var btnOTA = document.createElement("button");
    btnOTA.textContent = "OTA Update";
    btnOTA.setAttribute("id", "btn-ota");
    btnOTA.onclick = e => (window.location.href = baseUrl + "/www/update.html");

    var btnReset = document.createElement("button");
    btnReset.textContent = "Reboot";
    btnReset.setAttribute("id", "btn-reset");
    btnReset.onclick = function (e) {
        var cf = confirm("Are you sure to reboot board?");
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onload = function () {
                if (xhr.status < 400) {
                    alert('Board will reboot!');
                }
            };
            xhr.open("POST", baseUrl + "/j/reset_board");
            xhr.send();
            return true;
        }
        return false;
    };

    var hr = document.createElement('p');

    buttonDiv.append(btnLogs, btnRaw, btnClear, hr, btnFiles, btnOTA, btnReset);
    return buttonDiv;
}

function handleError(e) {
    var outputDiv = $('<div>').attr('id', 'output').attr('class', 'output');
    outputDiv.append($('<p>').text('Failed to load data.'));
    $('#content').append(outputDiv, buildButtonDiv());
}

function loadData() {
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

            $('#content').append(t, buildOutputDiv(d), buildFormDiv(d), buildButtonDiv());
        } else {
            handleError(e);
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