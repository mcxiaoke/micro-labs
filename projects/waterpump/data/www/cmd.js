const baseUrl = " http://192.168.1.135";

window.addEventListener("DOMContentLoaded", function (e) {
    console.log('DOMContentLoaded');
    $('#list-submit').on('click', function (e) {
        console.log('click');
        fetch(baseUrl + ($('#list-form').attr('action')), {
            credentials: 'include',
            method: 'GET',
            mode: 'cors',
        })
            .then(function (response) {
                return response.text();
            }).then(function (text) {
                console.log(text);
                var lines = text.split('\n').map((it) => $('<p>').text(it));
                console.log(lines);
                $('#output').append(lines.map((it) => it.html()));
            }).catch(function (error) {
                console.log(error);
            })

        return false;
    });
});