// const serverUrl = "http://192.168.1.134";
const serverUrl = "";
const baseUrl = "";

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

// in seconds
function humanElapsed(sec) {
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

var humanElapsedMs = (sec) => humanElapsed(sec / 1000);