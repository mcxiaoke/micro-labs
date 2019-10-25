function buildOutputDiv(d) {
    var tb = $("<table>").append(
        $("<tr>")
            .attr("id", "p-status")
            .append(
                $("<td>").text("Date: "),
                $("<td>").text(moment.unix(d["ts"]).format("MM-DD HH:mm:ss")),
                $("<td>").text("Status: "),
                $("<td>").text(d["on"] ? "Running" : "Idle")
            ),
        $("<tr>")
            .attr("id", "p-last")
            .append(
                $("<td>").text("Last RunAt: "),
                $("<td>").text(
                    moment.unix(d["lastAt2"]).format("MM-DD HH:mm:ss")
                ),
                $("<td>").text("Elapsed: "),
                $("<td>").text(d["lastElapsed"] + "s")
            ),
        $("<tr>")
            .attr("id", "p-next")
            .append(
                $("<td>").text("Next RunAt: "),
                $("<td>").text(
                    moment.unix(d["nextAt"]).format("MM-DD HH:mm:ss")
                ),
                $("<td>").text("Remains: "),
                $("<td>").text(humanElapsed(d["nextAt"] - moment().unix()))
            ),
        $("<tr>")
            .attr("id", "p-status")
            .append(
                $("<td>").text("Total Elapsed: "),
                $("<td>").text(d["totalElapsed"] + "s"),
                $("<td>").text("Up Time: "),
                $("<td>").text(humanElapsed(d["up"]))
            ),
        $("<tr>")
            .attr("id", "p-task")
            .append(
                $("<td>").text("Task Interval: "),
                $("<td>").text(humanElapsed(d["interval"] / 1000)),
                $("<td>").text("Task Duration: "),
                $("<td>").text(humanElapsed(d["duration"] / 1000))
            ),
        $("<tr>")
            .attr("id", "p-global")
            .append(
                $("<td>").text("Global Switch: "),
                $("<td>").text(d["switch"] ? "On" : "Off"),
                $("<td>").text("Debug: "),
                $("<td>").text(d["debug"] ? "True" : "False")
            ),
        $("<tr>")
            .attr("id", "p-sys")
            .append(
                $("<td>").text("Free Stack: "),
                $("<td>").text(d["stack"]),
                $("<td>").text("Free Heap: "),
                $("<td>").text(d["heap"])
            )
    );

    var o = $("<div>")
        .attr("id", "output")
        .attr("class", "output");
    return o.append(tb, $("<p>"));
}

function buildFormDiv(d) {
    var pf = $("<form>")
        .attr("id", "pump-form")
        .attr("method", "POST")
        .attr("action", serverUrl + "/api/" + (d["on"] ? "stop" : "start"))
        .append(
            $("<button>")
                .attr("id", "pump_submit")
                .attr("type", "submit")
                .text(d["on"] ? "Stop Pump" : "Start Pump")
        );

    var sf = $("<form>")
        .attr("id", "switch-form")
        .attr("method", "POST")
        .attr("action", serverUrl + "/api/" + (d["switch"] ? "off" : "on"))
        .append(
            $("<button>")
                .attr("id", "switch_submit")
                .attr("type", "submit")
                .text(d["switch"] ? "Switch Off" : "Switch On")
        );

    return $("<div>")
        .attr("id", "form-div")
        .append(pf, sf, $("<p>"));
}

function buildButtonDiv() {
    var buttonDiv = document.createElement("div");
    buttonDiv.setAttribute("id", "button-div");

    var btnLogs = document.createElement("button");
    btnLogs.textContent = "View Logs";
    btnLogs.setAttribute("id", "btn-logs");
    btnLogs.onclick = e => (window.location.href = "logs.html");

    var btnRaw = document.createElement("button");
    btnRaw.textContent = "Full Logs";
    btnRaw.setAttribute("id", "btn-raw");
    btnRaw.onclick = e => {
        window.open(
            serverUrl + "/file/log-" + moment().format("YYYY-MM-DD") + ".txt",
            "_blank"
        );
    };

    var btnClear = document.createElement("button");
    btnClear.textContent = "Clear Logs";
    btnClear.setAttribute("id", "btn-clear");
    // btnClear.setAttribute("type", "submit");
    btnClear.onclick = function(e) {
        e.preventDefault();
        var cf = confirm("Are you sure to delete all logs?");
        if (cf) {
            xhr = new XMLHttpRequest();
            xhr.onload = function() {
                if (xhr.status < 400) {
                    // alert("Server logs is cleared!");
                    console.log("Clear logs ok.");
                    // location.reload(true);
                }
            };
            xhr.open("POST", serverUrl + "/api/clear_logs");
            xhr.send();
            return true;
        }
        return false;
    };

    var btnFiles = document.createElement("button");
    btnFiles.textContent = "Files";
    btnFiles.setAttribute("id", "btn-files");
    btnFiles.onclick = e => (window.location.href = "files.html");

    var btnOTA = document.createElement("button");
    btnOTA.textContent = "Update";
    btnOTA.setAttribute("id", "btn-ota");
    btnOTA.onclick = e => (window.location.href = "update.html");

    var btnTimer = document.createElement("button");
    btnTimer.textContent = "Reset Timer";
    btnTimer.setAttribute("id", "btn-timer");
    btnTimer.onclick = function(e) {
        e.preventDefault();
        var cf = confirm("Are you sure to reset timer?");
        if (cf) {
            xhr = new XMLHttpRequest();
            xhr.onload = function() {
                console.log("timer reset.");
                // location.reload(true);
            };
            xhr.open("POST", serverUrl + "/api/timer");
            xhr.send();
            return true;
        }
        return false;
    };

    var btnReboot = document.createElement("button");
    btnReboot.textContent = "Reboot";
    btnReboot.setAttribute("id", "btn-reboot");
    btnReboot.onclick = function(e) {
        var cf = confirm("Are you sure to reboot board?");
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onload = function() {
                if (xhr.status < 400) {
                    alert("Board will reboot!");
                    setTimeout(() => {
                        location.reload(true);
                    }, 20000);
                }
            };
            xhr.open("POST", serverUrl + "/api/reboot");
            xhr.send();
            return true;
        }
        return false;
    };

    var hr = document.createElement("p");

    buttonDiv.append(
        btnLogs,
        btnRaw,
        btnClear,
        hr,
        btnFiles,
        btnOTA,
        btnTimer,
        btnReboot
    );
    return buttonDiv;
}

function handleError(firstTime) {
    if (firstTime) {
        var outputDiv = $("<div>")
            .attr("id", "output")
            .attr("class", "output");
        outputDiv.append($("<p>").text("Failed to load data."));
        $("#content").append(outputDiv, buildButtonDiv());
    }
}

function loadData(firstTime) {
    var xhr = new XMLHttpRequest();
    xhr.timeout = 3000;
    xhr.ontimeout = function(e) {
        console.log("ontimeout");
        handleError(firstTime);
    };
    xhr.onerror = function(e) {
        console.log("onerror");
        handleError(firstTime);
    };
    xhr.onloadend = function(e) {
        var output = document.createElement("div");
        output.innerHTML = document.getElementById("content").outerHTML;
        console.log(output);
    };
    xhr.onload = function(e) {
        console.log("onload " + xhr.status);
        if (xhr.status < 400) {
            var d = JSON.parse(xhr.responseText);
            console.log(d);
            var c = document.getElementById("content");
            var t = document.createElement("h1");
            t.textContent = "Pump Home";
            // c.append(t, buildOutputDiv(d), buildFormDiv(d), buildButtonDiv(d));
            $("#content").html("");
            $("#content").append(
                t,
                buildOutputDiv(d),
                buildFormDiv(d),
                buildButtonDiv()
            );
        } else {
            handleError(firstTime);
        }
    };
    xhr.open("GET", serverUrl + "/api/status");
    xhr.setRequestHeader("Accept", "application/json");
    xhr.send();
    return false;
}

function onReady(e) {
    loadData(true);
    setInterval(function() {
        loadData(false);
    }, 10000);
}
window.addEventListener("DOMContentLoaded", onReady);
