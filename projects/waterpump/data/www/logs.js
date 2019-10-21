function loadData() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', serverUrl + '/api/logs');
    xhr.onload = function () {
        if (xhr.status === 200) {
            var outputText = xhr.responseText.split(/\n/).map((it) => "<p>" + it + "</p>").join("\n");
            document.getElementById("output").innerHTML = outputText;
        } else {
            console.error("Failed: " + xhr.statusText);
            document.getElementById("output").innerHTML = "<p>Error: " + xhr.statusText + "</p>";
        }
    };
    xhr.send();
    return false;
}
window.addEventListener('DOMContentLoaded', (e) => {
    // call on dom loaded.
    loadData();
    $('#home').on('click', function(e){
        console.log('home click');
        window.location.href = 'index.html';
    })
    $('#reload').on('click', function (e) {
        loadData();
    });
    $('#clear').on('click', function (e) {
        var cf = confirm("Are you sure to delete all logs?");
        if (cf) {
            e.preventDefault();
            xhr = new XMLHttpRequest();
            xhr.onload = function () {
                if (xhr.status < 400) {
                    alert('Server logs is cleared!');
                    console.log("Clear logs ok.");
                }
                loadData();
            };
            xhr.open("POST", serverUrl + "/api/clear_logs");
            xhr.send();
            return true;
        }
        return false;
    })
});