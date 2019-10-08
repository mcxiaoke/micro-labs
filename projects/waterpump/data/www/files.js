const baseUrl = "http://192.168.100.7";

window.addEventListener("DOMContentLoaded", function (e) {
    console.log('DOMContentLoaded');
    $('#list-submit').on('click', function (e) {
        console.log('click');

        var xhr = new XMLHttpRequest();
        xhr.open('GET', baseUrl + $('#list-form').attr('action'));
        xhr.onload = function () {
            if (xhr.status < 400) {
                var lines = xhr.responseText.split('\n').map((it) => $('<p>').append(
                    $('<a>').attr('href', baseUrl + it).attr('target', '_blank').text(it))
                );
                lines.forEach((it) => $('#output').append(it));
                // $('#output').append(lines.map((it) => it.html()));
            } else {
                console.error("Failed: " + xhr.statusText);
                $("output").html("<p>Error: " + xhr.statusText + "</p>");
            }
        };
        xhr.send();
        return false;
    });
});