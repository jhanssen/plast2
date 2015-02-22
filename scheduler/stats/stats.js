/*global console, JSON, WebSocket*/

var ws;

var callbacks = {
    websocketOpen: function(evt) {
        console.log("ws open");
        ws.send("hello there");
    },
    websocketClose: function(evt) {
        console.log("ws close");
    },
    websocketMessage: function(evt) {
        console.log("ws msg " + evt.data);
    },
    websocketError: function(evt) {
        console.log("ws error " + JSON.stringify(evt));
    }
};

function init() {
    var url = "ws://" + window.location.hostname + ":" + window.location.port + "/";
    ws = new WebSocket(url);
    ws.onopen = callbacks.websocketOpen;
    ws.onclose = callbacks.websocketClose;
    ws.onmessage = callbacks.websocketMessage;
    ws.onerror = callbacks.websocketError;

    var canvas = document.getElementById('stats');
    paper.setup(canvas);
    var path = new paper.Path();
    path.strokeColor = 'black';
    var start = new paper.Point(100, 100);
    path.moveTo(start);
    path.lineTo(start.add([ 200, -50 ]));
    paper.view.draw();
}

window.addEventListener("load", init, false);
