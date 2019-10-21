const urlParams = new URLSearchParams(window.location.search);
const urlPath = urlParams.get('url');
const url = serverUrl + urlPath;

function loadData(e) {
    if(!urlPath){
        return;
    }
    $('#info').text('File Url: ' + url);
    var xhr = new XMLHttpRequest();
    xhr.onload = function (e) {
        if (xhr.status < 400) {
            var text = xhr.responseText;
            $('#text').text(text);
        } else {
            console.error("Failed: " + xhr.statusText);
            $("#info").text("Failed to load file: " + url);
        }
    };
    xhr.onerror = function (e) {
        console.error("Failed: " + e);
        $("#info").text("Failed to load file: " + url);
    }
    xhr.open('GET', url);
    xhr.send();
}

window.addEventListener("DOMContentLoaded", function (e) {
    console.log('DOMContentLoaded');
    loadData(e);
    $('#submit').on('click', function (e) {
        var cf = confirm('Are you sure to submit modification?');
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onload = function () {
                if (xhr.status < 400) {
                    alert('File updated!');
                }
            };
            var fd = new FormData();
            fd.append('file-path', urlPath);
            fd.append('file-data', $('#text').text());
            xhr.open("POST", serverUrl + "/api/view");
            // xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
            xhr.send(fd);
            return true;
        }
        return false;
    });
});