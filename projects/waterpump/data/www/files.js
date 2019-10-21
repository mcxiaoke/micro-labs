
function loadData(e) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        if (xhr.status < 400) {
            var files = xhr.responseText.split('\n').filter(Boolean);
            var lines = files.map(function (it) {
                var ca = $('<a>').attr('href', serverUrl + it).attr('target', '_blank').text(it);
                var aa = $('<button>').text('Delete');
                aa.on('click', function (e) {
                    var cf = confirm('Are you sure to delete file:[' + it + '] ?');
                    if (!cf) {
                        return false;
                    }
                    var ar = new XMLHttpRequest();
                    ar.onload = function () {
                        console.log(ar.statusText);
                        loadData();
                    }
                    var fd = new FormData();
                    fd.append('file_path', it);
                    ar.open('POST', serverUrl + '/api/delete_file');
                    ar.send(fd);
                });
                var da = $('<button>').text('Download');
                da.on('click', function (e) {
                    var link = document.createElement("a");
                    link.download = it.substring(it.lastIndexOf('/') + 1);
                    link.href = serverUrl + it;
                    document.body.appendChild(link);
                    link.click();
                    link.remove();
                    console.log(link.outerHTML);
                });
                var ea = $('<button>').text('View');
                ea.on('click', function (e) {
                    var url = 'edit.html?' + encodeURIComponent('url') + '=' + encodeURIComponent(it)
                    window.open(url, '_blank');
                });
                return $('<p>').append(ca, aa, da, ea);
            });
            $('#output').html('');
            lines.forEach((it) => $('#output').append(it));
            // $('#output').append(lines.map((it) => it.html()));
        } else {
            console.error("Failed: " + xhr.statusText);
            $("output").html("<p>Error: " + xhr.statusText + "</p>");
        }
    };
    xhr.onerror = function (e) {
        console.error("Failed: " + e);
    }
    xhr.open('GET', serverUrl + '/api/files');
    xhr.send();
}

window.addEventListener("DOMContentLoaded", function (e) {
    console.log('DOMContentLoaded');
    loadData(e);
    $('#reload').on('click', function (e) {
        console.log('click');
        loadData(e);
        return false;
    });
});