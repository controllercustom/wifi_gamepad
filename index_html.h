static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang=en>
<head>
<meta charset="UTF-8">
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>WiFi Gamepad 0.01</title>
<style type="text/css">
    table {
        display: table;
        table-layout: fixed;
        position: absolute;
        top: 0;
        bottom: 0;
        left: 0;
        right: 0;
        height: 100%;
        width: 100%;
        border-collapse: collapse;
    }

    td {
        border: 1px solid;
        font-size: 200%;
        text-align: center;
    }
</style>
<script type="text/javascript">
const WS_OPEN = 1;
const WS_CLOSED = 3;
let websock;
let websockQueue = [];
let connected = false;

function websock_send(json) {
  if (websock.readyState === WS_OPEN) {
      websock.send(json);
  } else {
    if (websock.readyState === WS_CLOSED) {
        websock = new WebSocket('ws://' + window.location.hostname + ':81/');
    }
    websockQueue.push(json);
  }
}

//////////////////////////////////////////////////////////////////////////////
//Keyboard Events
//////////////////////////////////////////////////////////////////////////////
document.addEventListener('keydown', function(event) {
  event.preventDefault()
  if (!event.repeat) {
    let json = JSON.stringify({event:event.type, code:event.code, key:event.key,
        charCode:event.charCode, keyCode:event.keyCode, altKey:event.altKey,
        ctrlKey:event.ctrlKey, metaKey:event.metaKey, shiftKey:event.shiftKey});
    websock_send(json);
  }
}, false);

document.addEventListener('keyup', function(event) {
  event.preventDefault()
  if (!event.repeat) {
    let json = JSON.stringify({event:event.type, code:event.code, key:event.key,
        charCode:event.charCode, keyCode:event.keyCode, altKey:event.altKey,
        ctrlKey:event.ctrlKey, metaKey:event.metaKey, shiftKey:event.shiftKey});
    websock_send(json);
  }
}, false);

//////////////////////////////////////////////////////////////////////////////
//Joystick/Gamepad Events
//////////////////////////////////////////////////////////////////////////////
window.addEventListener ("gamepadconnected",
  (e) => {
    let gp_connected = {
        event:"gamepad_connected",
        index:e.gamepad.index,
        id:e.gamepad.id
      };
    websock_send(JSON.stringify(gp_connected));
    console.log("gamepad connected", e.gamepad.id, "index", e.gamepad.index,
      "axes", e.gamepad.axes.length, "buttons", e.gamepad.buttons.length);
    let e_id = document.getElementById("gamepad_id_" + e.gamepad.index.toString());
    console.log("e_id", "gamepad_id_" + e.gamepad.index.toString());
    if (e_id) {
      e_id.innerHTML = e.gamepad.id;
    }
  },
  false,
);

window.addEventListener ("gamepaddisconnected",
  (e) => {
    let gp_disconnected = {
        event:"gamepad_disconnected",
        index:e.gamepad.index,
      };
    websock_send(JSON.stringify(gp_disconnected));
    console.log("gamepad disconnected", e.gamepad.id, "index", e.gamepad.index);
    let e_id = document.getElementById("gamepad_id_" + e.gamepad.index.toString());
    if (e_id) {
      e_id.innerHTML = "-";
    }
    let e_axes = document.getElementById("gamepad_axes_" + e.gamepad.index.toString());
    if (e_axes) {
      e_axes.innerHTML = "-";
    }
    let e_buttons = document.getElementById("gamepad_buttons_" + e.gamepad.index.toString());
    if (e_buttons) {
      e_buttons.innerHTML = "-";
    }
  },
  false,
);

setInterval(() => {
  const gamepads = navigator.getGamepads();
  gamepads.forEach((gp, index) => {
    if (gp !== null) {
      let gp_obj = {
        event:"gamepad",
        index:index,
        axes:[],
        buttons:[]
      };
      gp.buttons.forEach((button, bindex) => {
        gp_obj.buttons.push((button.pressed)?1:0);
      });

      gp.axes.forEach((axis, aindex) => {
        gp_obj.axes[aindex] = Math.round(32767 * axis);
      });
      websock_send(JSON.stringify(gp_obj));

      // Update HTML table
      let e_buttons = document.getElementById("gamepad_buttons_" + index.toString());
      if (e_buttons) {
        e_buttons.innerHTML = JSON.stringify(gp_obj.buttons).replace(/,/g, ", ");
      }
      let e_axes = document.getElementById("gamepad_axes_" + index.toString());
      if (e_axes) {
        e_axes.innerHTML = JSON.stringify(gp_obj.axes).replace(/,/g, ", ");
      }
    }
  });
}, 8)

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) {
    console.log('websock onopen', evt);
    connected = true;
    for (let i = 0; i < websockQueue.length; i++) {
      websock.send(websockQueue[i]);
    }
    websockQueue = [];
  };
  websock.onclose = function(evt) {
    console.log('websock onclose', evt);
    connected = false;
  };
  websock.onerror = function(evt) {
    console.log('websock onerror', evt);
    connected = false;
  };
  websock.onmessage = function(evt) {
    console.log('websock onmessage', evt);
  };
}

document.addEventListener("DOMContentLoaded", start);

</script>
</head>
<body>
<table>
<thead>
<tr>
<th>Index</th>
<th>ID</th>
<th>Axes</th>
<th>Buttons</th>
</tr>
</thead>
<tbody>
<tr>
<td>0</td>
<td id="gamepad_id_0">-</td>
<td id="gamepad_axes_0">-</td>
<td id="gamepad_buttons_0">-</td>
</tr>
<tr>
<td>1</td>
<td id="gamepad_id_1">-</td>
<td id="gamepad_axes_1">-</td>
<td id="gamepad_buttons_1">-</td>
</tr>
<tr>
<td>2</td>
<td id="gamepad_id_2">-</td>
<td id="gamepad_axes_2">-</td>
<td id="gamepad_buttons_2">-</td>
</tr>
<tr>
<td>3</td>
<td id="gamepad_id_3">-</td>
<td id="gamepad_axes_3">-</td>
<td id="gamepad_buttons_3">-</td>
</tr>
</tbody>
</table>
</body>
</html>
)rawliteral";
